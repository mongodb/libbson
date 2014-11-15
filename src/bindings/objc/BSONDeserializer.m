//
//  BSONDeserializer.m
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import "BSONDeserializer.h"
#import "BSON_OrderedDictionary.h"
#import "BSONDocument.h"
#import "ObjCBSON.h"

@interface BSONDocument (Module)
- (const bson_t *) nativeValue;
- (id) initWithNativeValue:(bson_t *) bson;
@end

@interface BSONObjectID (Module)
- (id) initWithNativeValue:(const bson_oid_t *) objectIDPointer;
@end

#pragma mark - BSONDeserializer callbacks

bool visit_before(const bson_iter_t *iter,
                  const char        *key,
                  void              *data) {
    return NO;
}

bool visit_after(const bson_iter_t *iter,
                 const char        *key,
                 void              *data) {
    return NO;
}

void visit_corrupt(const bson_iter_t *iter,
                   void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    [deserializer markCorrupt];
}

bool visit_double(const bson_iter_t *iter,
                  const char        *key,
                  double             v_double,
                  void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSNumber numberWithDouble:v_double];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_utf8(const bson_iter_t *iter,
                const char        *key,
                size_t             v_utf8_len,
                const char        *v_utf8,
                void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [[NSString alloc] initWithBytes:v_utf8
                                        length:v_utf8_len
                                      encoding:NSUTF8StringEncoding];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_document(const bson_iter_t *iter,
                    const char        *key,
                    const bson_t      *v_document,
                    void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    
    NSError *error = nil;
    id value = [BSONDeserializer dictionaryWithNativeDocument:v_document error:&error];
    if (!value) {
        deserializer.error = error;
        return YES;
    }
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

NSMutableArray * NSArrayFromDictionary(NSDictionary * dictionary) {
    NSArray *sortedKeys = [dictionary.allKeys sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        return [obj1 compare:obj2 options:NSNumericSearch];
    }];
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:dictionary.count];
    for (NSString *key in sortedKeys) [array addObject:[dictionary objectForKey:key]];
    return array;
}

bool visit_array(const bson_iter_t *iter,
                 const char        *key,
                 const bson_t      *v_array,
                 void              *data) {
    // This alocates a dictionary that is not needed; could be slightly more efficient
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    NSError *error = nil;
    id dictionary = [BSONDeserializer dictionaryWithNativeDocument:v_array error:&error];
    if (!dictionary) {
        deserializer.error = error;
        return YES;
    }
    id value = NSArrayFromDictionary(dictionary);
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_binary(const bson_iter_t *iter,
                  const char        *key,
                  bson_subtype_t     v_subtype,
                  size_t             v_binary_len,
                  const uint8_t     *v_binary,
                  void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [[NSData alloc] initWithBytes:v_binary length:v_binary_len];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_undefined(const bson_iter_t *iter,
                     const char        *key,
                     void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [BSONUndefined undefined];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_oid(const bson_iter_t *iter,
               const char        *key,
               const bson_oid_t  *v_oid,
               void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [[BSONObjectID alloc] initWithNativeValue:v_oid];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_bool(const bson_iter_t *iter,
                const char        *key,
                bool               v_bool,
                void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSNumber numberWithBool:v_bool];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_date_time(const bson_iter_t *iter,
                     const char        *key,
                     int64_t            msec_since_epoch,
                     void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSDate dateWithTimeIntervalSince1970:msec_since_epoch / 1000.f];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_null(const bson_iter_t *iter,
                const char        *key,
                void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSNull null];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_regex(const bson_iter_t *iter,
                 const char        *key,
                 const char        *v_regex,
                 const char        *v_options,
                 void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONRegularExpression *value = [[BSONRegularExpression alloc] init];
    value.pattern = [NSString stringWithUTF8String:v_regex];
    value.options = [NSString stringWithUTF8String:v_options];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_dbpointer(const bson_iter_t *iter,
                     const char        *key,
                     size_t             v_collection_len,
                     const char        *v_collection,
                     const bson_oid_t  *v_oid,
                     void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONDatabasePointer *value = [[BSONDatabasePointer alloc] init];
    value.collection = [[NSString alloc] initWithBytes:v_collection
                                                length:v_collection_len
                                              encoding:NSUTF8StringEncoding];
    value.objectID = [[BSONObjectID alloc] initWithNativeValue:v_oid];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_code(const bson_iter_t *iter,
                const char        *key,
                size_t             v_code_len,
                const char        *v_code,
                void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONCode *value = [[BSONCode alloc] init];
    value.code = [[NSString alloc] initWithBytes:v_code
                                          length:v_code_len
                                        encoding:NSUTF8StringEncoding];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_symbol(const bson_iter_t *iter,
                  const char        *key,
                  size_t             v_symbol_len,
                  const char        *v_symbol,
                  void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONSymbol *value = [[BSONSymbol alloc] init];
    value.symbol = [[NSString alloc] initWithBytes:v_symbol
                                            length:v_symbol_len
                                          encoding:NSUTF8StringEncoding];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_codewscope(const bson_iter_t *iter,
                      const char        *key,
                      size_t             v_code_len,
                      const char        *v_code,
                      const bson_t      *v_scope,
                      void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONCode *value = [[BSONCodeWithScope alloc] init];
    value.code = [[NSString alloc] initWithBytes:v_code
                                          length:v_code_len
                                        encoding:NSUTF8StringEncoding];
    // FIXME!
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_int32(const bson_iter_t *iter,
                 const char        *key,
                 int32_t            v_int32,
                 void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSNumber numberWithInt:v_int32];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_timestamp(const bson_iter_t *iter,
                     const char        *key,
                     uint32_t           v_timestamp,
                     uint32_t           v_increment,
                     void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    BSONTimestamp *value = [[BSONTimestamp alloc] init];
    value.timeInSeconds = v_timestamp;
    value.increment = v_increment;
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_int64(const bson_iter_t *iter,
                 const char        *key,
                 int64_t            v_int64,
                 void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [NSNumber numberWithLongLong:v_int64];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_maxkey(const bson_iter_t *iter,
                  const char        *key,
                  void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [BSONMaxKey maxKey];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_minkey(const bson_iter_t *iter,
                  const char        *key,
                  void              *data) {
    BSONDeserializer *deserializer = (__bridge BSONDeserializer *)data;
    id value = [BSONMinKey minKey];
    [deserializer appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bson_visitor_t visitor_table = {
    visit_before,
    visit_after,
    visit_corrupt,
    visit_double,
    visit_utf8,
    visit_document,
    visit_array,
    visit_binary,
    visit_undefined,
    visit_oid,
    visit_bool,
    visit_date_time,
    visit_null,
    visit_regex,
    visit_dbpointer,
    visit_code,
    visit_symbol,
    visit_codewscope,
    visit_int32,
    visit_timestamp,
    visit_int64,
    visit_maxkey,
    visit_minkey
};

#pragma mark - BSONDeserializer

@interface BSONDeserializer ()
@property (assign) bson_iter_t *_iter;
@end

@implementation BSONDeserializer

- (id) initWithNativeIterator:(bson_iter_t *) iter {
    if (self = [super init]) {
        self._iter = iter;
        self.dictionary = [BSON_OrderedDictionary dictionary];
    }
    return self;
}

- (void) appendObject:(id) object forKey:(NSString *) key {
    [self.dictionary setObject:object forKey:key];
}

- (void) markCorrupt {
    self.corrupt = YES;
}

- (void) start {
    bson_iter_visit_all(self._iter, &visitor_table, (__bridge void *)(self));
}

+ (NSDictionary *) dictionaryWithNativeDocument:(const bson_t *) bson error:(NSError **) error {
    bson_iter_t iter;
    bson_iter_init(&iter, bson);
    BSONDeserializer *deserializer = [[BSONDeserializer alloc] initWithNativeIterator:&iter];
    [deserializer start];
    if (deserializer.corrupt) {
        if (error) {
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"Document is corrupt" };
            *error = [NSError errorWithDomain:BSONErrorDomain code:BSONIsCorrupt userInfo:userInfo];
        }
        return nil;
    }
    return deserializer.dictionary;
}

@end
