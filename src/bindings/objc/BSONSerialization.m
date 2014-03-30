//
//  BSONVisitor.m
//  libbson
//
//  Created by Paul Melnikow on 3/29/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import "BSONSerialization.h"
#import "BSON_OrderedDictionary.h"
#import "BSONTypes.h"
#import "BSON_Helper.h"
#import "BSONDocument.h"
#import "ObjCBSON.h"
#import <bson.h>

#pragma mark - Module interfaces

@interface BSONObjectID (Module)
- (id) initWithNativeValue:(const bson_oid_t *) objectIDPointer;
@end

@interface BSONDocument (Module)
- (const bson_t *) nativeValue;
- (id) initWithNativeValue:(bson_t *) bson;
@end

@interface BSONSerialization (Module)
+ (NSDictionary *) dictionaryWithDocument:(BSONDocument *) document error:(NSError **) error;
+ (BSONDocument *) BSONDocumentWithDictionary:(NSDictionary *) dictionary error:(NSError **) error;
@end

#pragma mark - BSONVisitor data structure

@interface BSONVisitor : NSObject

- (void) appendObject:(id) object forKey:(NSString *) key;
- (void) markCorrupt;

@property (strong) BSON_OrderedDictionary *dictionary;
@property (assign) BOOL corrupt;
@property (strong) NSError *error;

@end

@implementation BSONVisitor

- (id) init {
    if (self = [super init]) {
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

@end

#pragma mark - BSONVisitor callbacks

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
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    [visitor markCorrupt];
}

bool visit_double(const bson_iter_t *iter,
                  const char        *key,
                  double             v_double,
                  void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSNumber numberWithDouble:v_double];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_utf8(const bson_iter_t *iter,
                const char        *key,
                size_t             v_utf8_len,
                const char        *v_utf8,
                void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [[NSString alloc] initWithBytes:v_utf8
                                        length:v_utf8_len
                                      encoding:NSUTF8StringEncoding];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_document(const bson_iter_t *iter,
                    const char        *key,
                    const bson_t      *v_document,
                    void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    
    BSONDocument *child = [[BSONDocument alloc] initWithNativeValue:(bson_t *)v_document];
    NSError *error = nil;
    id value = [BSONSerialization dictionaryWithDocument:child error:&error];
    if (!value) {
        visitor.error = error;
        return YES;
    }
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
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
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONDocument *child = [[BSONDocument alloc] initWithNativeValue:(bson_t *)v_array];
    NSError *error = nil;
    id dictionary = [BSONSerialization dictionaryWithDocument:child error:&error];
    if (!dictionary) {
        visitor.error = error;
        return YES;
    }
    id value = NSArrayFromDictionary(dictionary);
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_binary(const bson_iter_t *iter,
                  const char        *key,
                  bson_subtype_t     v_subtype,
                  size_t             v_binary_len,
                  const uint8_t     *v_binary,
                  void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [[NSData alloc] initWithBytes:v_binary length:v_binary_len];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_undefined(const bson_iter_t *iter,
                     const char        *key,
                     void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [BSONUndefined undefined];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_oid(const bson_iter_t *iter,
               const char        *key,
               const bson_oid_t  *v_oid,
               void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [[BSONObjectID alloc] initWithNativeValue:v_oid];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_bool(const bson_iter_t *iter,
                const char        *key,
                bool               v_bool,
                void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSNumber numberWithBool:v_bool];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_date_time(const bson_iter_t *iter,
                     const char        *key,
                     int64_t            msec_since_epoch,
                     void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSDate dateWithTimeIntervalSince1970:msec_since_epoch / 1000.f];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_null(const bson_iter_t *iter,
                const char        *key,
                void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSNull null];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_regex(const bson_iter_t *iter,
                 const char        *key,
                 const char        *v_regex,
                 const char        *v_options,
                 void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONRegularExpression *value = [[BSONRegularExpression alloc] init];
    value.pattern = [NSString stringWithUTF8String:v_regex];
    value.options = [NSString stringWithUTF8String:v_options];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_dbpointer(const bson_iter_t *iter,
                     const char        *key,
                     size_t             v_collection_len,
                     const char        *v_collection,
                     const bson_oid_t  *v_oid,
                     void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONDatabasePointer *value = [[BSONDatabasePointer alloc] init];
    value.collection = [[NSString alloc] initWithBytes:v_collection
                                                length:v_collection_len
                                              encoding:NSUTF8StringEncoding];
    value.objectID = [[BSONObjectID alloc] initWithNativeValue:v_oid];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_code(const bson_iter_t *iter,
                const char        *key,
                size_t             v_code_len,
                const char        *v_code,
                void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONCode *value = [[BSONCode alloc] init];
    value.code = [[NSString alloc] initWithBytes:v_code
                                          length:v_code_len
                                        encoding:NSUTF8StringEncoding];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_symbol(const bson_iter_t *iter,
                  const char        *key,
                  size_t             v_symbol_len,
                  const char        *v_symbol,
                  void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONSymbol *value = [[BSONSymbol alloc] init];
    value.symbol = [[NSString alloc] initWithBytes:v_symbol
                                            length:v_symbol_len
                                          encoding:NSUTF8StringEncoding];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_codewscope(const bson_iter_t *iter,
                      const char        *key,
                      size_t             v_code_len,
                      const char        *v_code,
                      const bson_t      *v_scope,
                      void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONCode *value = [[BSONCodeWithScope alloc] init];
    value.code = [[NSString alloc] initWithBytes:v_code
                                          length:v_code_len
                                        encoding:NSUTF8StringEncoding];
    // FIXME!
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_int32(const bson_iter_t *iter,
                 const char        *key,
                 int32_t            v_int32,
                 void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSNumber numberWithInt:v_int32];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_timestamp(const bson_iter_t *iter,
                     const char        *key,
                     uint32_t           v_timestamp,
                     uint32_t           v_increment,
                     void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    BSONTimestamp *value = [[BSONTimestamp alloc] init];
    value.timeInSeconds = v_timestamp;
    value.increment = v_increment;
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_int64(const bson_iter_t *iter,
                 const char        *key,
                 int64_t            v_int64,
                 void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [NSNumber numberWithLongLong:v_int64];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_maxkey(const bson_iter_t *iter,
                  const char        *key,
                  void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [BSONMaxKey maxKey];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
    return NO;
}

bool visit_minkey(const bson_iter_t *iter,
                  const char        *key,
                  void              *data) {
    BSONVisitor *visitor = (__bridge BSONVisitor *)data;
    id value = [BSONMinKey minKey];
    [visitor appendObject:value forKey:[NSString stringWithUTF8String:key]];
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

@implementation BSONSerialization

+ (NSDictionary *) dictionaryWithDocument:(BSONDocument *) document error:(NSError **) error {
    bson_iter_t iter;
    bson_iter_init(&iter, document.nativeValue);
    BSONVisitor *visitor = [[BSONVisitor alloc] init];
    bson_iter_visit_all(&iter, &visitor_table, (__bridge void *)(visitor));
    if (visitor.corrupt) {
        if (error) {
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"Document is corrupt" };
            *error = [NSError errorWithDomain:BSONErrorDomain code:BSONIsCorrupt userInfo:userInfo];
        }
        return nil;
    }
    return visitor.dictionary;
}

+ (NSDictionary *) dictionaryWithBSONData:(NSData *) data error:(NSError **) error {
    bson_raise_if_nil(data);
    BSONDocument *document = [BSONDocument documentWithData:data noCopy:YES];
    if (document == nil) {
        if (error) {
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"Unable to create document" };
            *error = [NSError errorWithDomain:BSONErrorDomain code:-1 userInfo:userInfo];
        }
        return nil;
    }
    return [self dictionaryWithDocument:document error:error];
}

+ (BSONDocument *) BSONDocumentWithDictionary:(NSDictionary *) dictionary error:(NSError **) error {
    bson_raise_if_nil(dictionary);
    BSONDocument *document = [BSONDocument document];
    for (NSString *key in [dictionary allKeys]) {
        id obj = [dictionary objectForKey:key];
        if (![document appendObject:obj forKey:key error:error]) {
            return nil;
        }
    }
    return document;
}

+ (NSData *) BSONDataWithDictionary:(NSDictionary *) dictionary error:(NSError **) error {
    return [[self BSONDocumentWithDictionary:dictionary error:error] dataValue];
}

@end
