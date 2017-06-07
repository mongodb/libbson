#include <bson.h>
#include "TestSuite.h"
#include "json-test.h"
#include "corpus-test.h"

#ifndef JSON_DIR
#define JSON_DIR "tests/json"
#endif


typedef struct {
   const char *scenario;
   const char *test;
} skipped_corpus_test_t;

skipped_corpus_test_t SKIPPED_CORPUS_TESTS[] = {
   /* CDRIVER-1879, can't make Code with embedded NIL */
   {"Javascript Code", "Embedded nulls"},
   {"Javascript Code with Scope",
    "Unicode and embedded null in code string, empty scope"},
   {0}};


static void
compare_data (const uint8_t *a,
              uint32_t a_len,
              const uint8_t *b,
              uint32_t b_len)
{
   bson_string_t *a_str;
   bson_string_t *b_str;
   uint32_t i;

   if (a_len != b_len || memcmp (a, b, (size_t) a_len)) {
      a_str = bson_string_new (NULL);
      for (i = 0; i < a_len; i++) {
         bson_string_append_printf (a_str, "%02X", (int) a[i]);
      }

      b_str = bson_string_new (NULL);
      for (i = 0; i < b_len; i++) {
         bson_string_append_printf (b_str, "%02X", (int) b[i]);
      }

      fprintf (stderr,
               "unequal data of length %d and %d:\n%s\n%s\n",
               a_len,
               b_len,
               a_str->str,
               b_str->str);

      abort ();
   }
}


/*
See:
github.com/mongodb/specifications/blob/master/source/bson-corpus/bson-corpus.rst
#testing-validity

* for cB input:
    * bson_to_canonical_extended_json(cB) = cE
    * bson_to_relaxed_extended_json(cB) = rE (if rE exists)

* for cE input:
    * json_to_bson(cE) = cB (unless lossy)

* for dB input (if it exists):
    * bson_to_canonical_extended_json(dB) = cE
    * bson_to_relaxed_extended_json(dB) = rE (if rE exists)

* for rE input (if it exists):
    bson_to_relaxed_extended_json( json_to_bson(rE) ) = rE

 */
static void
test_bson_corpus (test_bson_type_t *test)
{
   skipped_corpus_test_t *skip;
   bson_t cB;
   bson_t dB;
   bson_t *decode_cE;
   bson_t *decode_dE;
   bson_t *decode_rE;
   bson_error_t error;

   BSON_ASSERT (test->cB);
   BSON_ASSERT (test->cE);

   for (skip = SKIPPED_CORPUS_TESTS; skip->scenario != NULL; skip++) {
      if (!strcmp (skip->scenario, test->scenario_description) &&
          !strcmp (skip->test, test->test_description)) {
         if (test_suite_debug_output ()) {
            printf ("      SKIP\n");
            fflush (stdout);
         }

         return;
      }
   }

   BSON_ASSERT (bson_init_static (&cB, test->cB, test->cB_len));
   ASSERT_CMPJSON (bson_as_extended_json (&cB, NULL), test->cE);

   if (test->rE) {
      ASSERT_CMPJSON (bson_as_json (&cB, NULL), test->rE);
   }

   decode_cE =
      bson_new_from_json ((const uint8_t *) test->cE, -1, &error);

   ASSERT_OR_PRINT (decode_cE, error);

   if (!test->lossy) {
      compare_data (
         bson_get_data (decode_cE), decode_cE->len, test->cB, test->cB_len);
   }

   if (test->dB) {
      BSON_ASSERT (bson_init_static (&dB, test->dB, test->dB_len));
      ASSERT_CMPJSON (bson_as_extended_json (&dB, NULL), test->cE);

      if (test->rE) {
         ASSERT_CMPJSON (bson_as_json (&dB, NULL), test->rE);
      }

      bson_destroy (&dB);
   }

   if (test->dE) {
      decode_dE =
         bson_new_from_json ((const uint8_t *) test->dE, -1, &error);

      ASSERT_OR_PRINT (decode_dE, error);
      ASSERT_CMPJSON (bson_as_json (decode_dE, NULL), test->cE);

      bson_destroy (decode_dE);
   }

   if (test->rE) {
      decode_rE =
         bson_new_from_json ((const uint8_t *) test->rE, -1, &error);

      ASSERT_OR_PRINT (decode_rE, error);
      ASSERT_CMPJSON (bson_as_json (decode_rE, NULL), test->rE);

      bson_destroy (decode_rE);
   }

   bson_destroy (decode_cE);
   bson_destroy (&cB);
}


static void
test_bson_corpus_cb (bson_t *scenario)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;
   bson_t invalid_bson;

   /* test valid BSON and Extended JSON */
   corpus_test (scenario, test_bson_corpus);

   /* test invalid BSON */
   if (bson_iter_init_find (&iter, scenario, "decodeErrors")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test;
         const char *description;
         uint8_t *bson_str = NULL;
         uint32_t bson_str_len = 0;

         bson_iter_recurse (&inner_iter, &test);
         while (bson_iter_next (&test)) {
            if (!strcmp (bson_iter_key (&test), "description")) {
               description = bson_iter_utf8 (&test, NULL);
               corpus_test_print_description (description);
            }

            if (!strcmp (bson_iter_key (&test), "bson")) {
               bson_str = corpus_test_unhexlify (&test, &bson_str_len);
            }
         }

         ASSERT (bson_str);
         ASSERT (!bson_init_static (&invalid_bson, bson_str, bson_str_len) ||
                 bson_empty (&invalid_bson) ||
                 !bson_as_extended_json (&invalid_bson, NULL));

         bson_free (bson_str);
      }
   }
}

void
test_bson_corpus_install (TestSuite *suite)
{
   install_json_test_suite (
      suite, JSON_DIR "/bson_corpus", test_bson_corpus_cb);
}
