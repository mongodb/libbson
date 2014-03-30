//
//  BSONDocument.m
//  libbson
//
//  Created by Paul Melnikow on 3/28/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import "BSONDocument.h"
#import "BSON_Helper.h"
#import "BSONSerialization.h"
#import "ObjCBSON.h"
#import <bson.h>


@interface BSONDocument ()
@property (strong) NSData *inData;
@end

@interface BSONDocument (Module)
- (const bson_t *) nativeValue;
- (id) initWithNativeValue:(bson_t *) bson;
@end

@interface BSONObjectID (Module)
- (bson_oid_t) nativeValue;
@end

@interface BSONSerialization (Module)
+ (NSDictionary *) dictionaryWithDocument:(BSONDocument *) document error:(NSError **) error;
+ (BSONDocument *) BSONDocumentWithDictionary:(NSDictionary *) dictionary error:(NSError **) error;
@end

@implementation BSONDocument {
    bson_t *_bson;
}

- (id) init {
    if (self = [super init]) {
        _bson = bson_new();
    }
    return self;
}

- (id) initWithCapacity:(NSUInteger) bytes {
    if (bytes > UINT32_MAX)
        return self = nil;
    if (self = [super init]) {
        _bson = bson_sized_new(bytes);
    }
    return self;
}

- (id) initWithData:(NSData *) data noCopy:(BOOL) noCopy {
    if (self = [super init]) {
        if (data.length > UINT32_MAX)
            return self = nil;
        if (noCopy)
            self.inData = data;
        else
            self.inData = [data isKindOfClass:[NSMutableData class]] ? [NSData dataWithData:data] : data;
        _bson = bson_new();
        if (!bson_init_static(_bson, self.inData.bytes, (uint32_t)self.inData.length)) {
            bson_destroy(_bson);
            return self = nil;
        }
    }
    return self;
}

- (id) initWithNativeValue:(bson_t *) bson {
    if (bson == NULL) return self = nil;
    if (self = [super init]) {
        _bson = bson;
    }
    return self;
}

+ (instancetype) document {
    return [[self alloc] init];
}

+ (instancetype) documentWithCapacity:(NSUInteger) bytes {
    return [[self alloc] initWithCapacity:bytes];
}

+ (instancetype) documentWithData:(NSData *) data {
    return [[self alloc] initWithData:data noCopy:NO];
}

+ (instancetype) documentWithData:(NSData *) data noCopy:(BOOL) noCopy {
    return [[self alloc] initWithData:data noCopy:noCopy];
}

- (void) dealloc {
    bson_destroy(_bson);
    _bson = NULL;
}

#pragma mark -

- (instancetype) copy {
    bson_t *bson = bson_copy(self.nativeValue);
    return [[self.class alloc] initWithNativeValue:bson];
}

- (void) reinit {
    if (self.inData) {
        // If this was initialized using static data, just clean up and
        // then create a new object.
        bson_destroy(_bson);
        _bson = NULL;
        self.inData = nil;
        _bson = bson_new();
    } else {
        bson_reinit(_bson);
    }
}

#pragma mark -

- (const bson_t *) nativeValue {
    return _bson;
}

- (NSData *) dataValue {
    if (self.inData) return self.inData;
    return [NSData dataWithBytes:(void *)bson_get_data(_bson) length:_bson->len];
}

- (NSData *) dataValueNoCopy {
    if (self.inData) return self.inData;
    return [NSData dataWithBytesNoCopy:(void *)bson_get_data(_bson) length:_bson->len];
}

- (NSString *) description {
    size_t length = 0;
    char * json = bson_as_json(_bson, &length);
    return [[NSString alloc] initWithBytesNoCopy:json
                                          length:length
                                        encoding:NSUTF8StringEncoding
                                    freeWhenDone:YES];
}

#pragma mark -

+ (NSUInteger) maximumCapacity {
    return BSON_MAX_SIZE;
}

- (BOOL) isEmpty {
    return bson_empty(_bson);
}

- (BOOL) hasField:(NSString *) key {
    return bson_has_field(_bson, key.UTF8String);
}

- (NSComparisonResult) compare:(BSONDocument *) other {
    return bson_NSComparisonResultFromQsort(bson_compare(_bson, other.nativeValue));
}

- (BOOL) isEqual:(BSONDocument *) document {
    if (![document isKindOfClass:[self class]]) return NO;
    return bson_equal(_bson, document.nativeValue);
}

- (BOOL) appendData:(NSData *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(NSData);
    if (value.length > UINT32_MAX) [NSException raise:NSInvalidArgumentException format:@"Data is too long"];
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_BINARY(_bson, key.UTF8String, 0, value.bytes, (uint32_t)value.length);
}

- (BOOL) appendBool:(BOOL) value forKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_BOOL(_bson, key.UTF8String, value);
}

- (BOOL) appendCode:(BSONCode *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONCode);
    bson_raise_if_nil(value.code)
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_CODE(_bson, key.UTF8String, value.code.UTF8String);
}

- (BOOL) appendCodeWithScope:(BSONCodeWithScope *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONCodeWithScope);
    bson_raise_if_nil(value.code);
    bson_raise_if_nil(value.scope);
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_CODE_WITH_SCOPE(_bson, key.UTF8String, value.code.UTF8String, value.scope.nativeValue);
}

- (BOOL) appendDatabasePointer:(BSONDatabasePointer *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONDatabasePointer);
    bson_raise_if_nil(value.collection);
    bson_raise_if_nil(value.objectID);
    bson_raise_if_string_too_long(value.collection, @"Collection name is too long");
    bson_raise_if_key_nil_or_too_long();
    const char * utf8 = key.UTF8String;
    bson_oid_t objectIdNative = value.objectID.nativeValue;
    return bson_append_dbpointer(_bson, utf8, (int)strlen(utf8), value.collection.UTF8String, &objectIdNative);
}

- (BOOL) appendDouble:(double) value forKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_DOUBLE(_bson, key.UTF8String, value);
}

- (BOOL) appendEmbeddedDocument:(BSONDocument *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONDocument);
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_DOCUMENT(_bson, key.UTF8String, value.nativeValue);
}

- (BOOL) appendInt32:(int32_t) value forKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_INT32(_bson, key.UTF8String, value);
}

- (BOOL) appendInt64:(int64_t) value forKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_INT64(_bson, key.UTF8String, value);
}

- (BOOL) appendMinKeyForKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_MINKEY(_bson, key.UTF8String);
}

- (BOOL) appendMaxKeyForKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_MAXKEY(_bson, key.UTF8String);
}

- (BOOL) appendNullForKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_NULL(_bson, key.UTF8String);
}

- (BOOL) appendObjectID:(BSONObjectID *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONObjectID);
    bson_raise_if_key_nil_or_too_long();
    bson_oid_t objectIDNative = value.nativeValue;
    return BSON_APPEND_OID(_bson, key.UTF8String, &objectIDNative);
}

- (BOOL) appendRegularExpression:(BSONRegularExpression *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONRegularExpression);
    bson_raise_if_nil(value.pattern);
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_REGEX(_bson, key.UTF8String, value.pattern.UTF8String, [value.options ?: @"" UTF8String]);
}

- (BOOL) appendString:(NSString *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(NSString);
    bson_raise_if_string_too_long(value, @"Value is too long");
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_UTF8(_bson, key.UTF8String, value.UTF8String);
}

- (BOOL) appendSymbol:(BSONSymbol *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONSymbol);
    bson_raise_if_nil(value.symbol);
    bson_raise_if_string_too_long(value.symbol, @"Value is too long");
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_SYMBOL(_bson, key.UTF8String, value.symbol.UTF8String);
}

- (BOOL) appendDate:(NSDate *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(NSDate);
    bson_raise_if_key_nil_or_too_long();
    int64_t millis = 1000.f * [value timeIntervalSince1970];
    return BSON_APPEND_DATE_TIME(_bson, key.UTF8String, millis);
}

- (BOOL) appendTimestamp:(BSONTimestamp *) value forKey:(NSString *) key {
    bson_raise_if_value_is_not_instance_of_class(BSONTimestamp);
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_TIMESTAMP(_bson, key.UTF8String, value.timeInSeconds, value.increment);
}

- (BOOL) appendUndefinedForKey:(NSString *) key {
    bson_raise_if_key_nil_or_too_long();
    return BSON_APPEND_UNDEFINED(_bson, key.UTF8String);
}

#pragma mark - Generalized serialization methods

+ (Class) _booleanClass {
    // The boolean class is private, so we acesss it this way.
    static Class singleton;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        singleton = [[NSNumber numberWithBool:YES] class];
    });
    return singleton;
}

- (BOOL) _appendNumber:(NSNumber *) number forKey:(NSString *) key error:(NSError **) error {
    // Booleans are a special case. NSNumber stores them as signed chars.
    // CFNumberGetType((CFNumberRef) [NSNumber numberWithBool:YES]) returns kCFNumberCharType
    // [[NSNumber numberWithBool:YES] objCType] returns "c"
    // These are the same values as for [NSNumber numberWithChar:'a']
    // So instead, we check its class.
    if ([number isKindOfClass:[self.class _booleanClass]]) {
        return [self appendBool:number.boolValue forKey:key];
    }

    switch (*(number.objCType)) {
        case 'd':
        case 'f':
            return [self appendDouble:number.doubleValue forKey:key];
        case 'l':
        case 'L':
        case 'q':
            return [self appendInt64:number.integerValue forKey:key];
        case 'Q':
            if ((uint64_t)number.unsignedIntegerValue > INT64_MAX) {
                if (error) {
                    NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"Unsigned integer value overflows BSON signed integer capacity" };
                    *error = [NSError errorWithDomain:BSONErrorDomain code:BSONIntegerOverflow userInfo:userInfo];
                }
                return NO;
            }
            return [self appendInt64:number.integerValue forKey:key];
        case 'B': // C++/C99 bool
            return [self appendBool:number.boolValue forKey:key];
        case 'c': // Used for ObjC numberWithBool: but also numberWithChar:
        case 'C':
        case 's':
        case 'S':
        case 'i':
        case 'I':
            return [self appendInt32:number.intValue forKey:key];
        default:
            [NSException raise:NSInvalidArgumentException format:@"Unexpected objCType: %s", number.objCType];
            return NO;
    }
}

- (BOOL) _appendDictionary:(NSDictionary *) dictionary forKey:(NSString *) key error:(NSError **) error {
    bson_raise_if_key_nil_or_too_long();
    BOOL success = YES;
    bson_t child;
    const char *utf8 = [key UTF8String];
    bson_append_document_begin(_bson, utf8, (int)strlen(utf8), &child);
    BSONDocument *childDocument = [[BSONDocument alloc] initWithNativeValue:&child];
    for (NSString *key in dictionary.allKeys) {
        id obj = [dictionary objectForKey:key];
        if (![childDocument appendObject:obj forKey:key error:error]) {
            success = NO;
            break;
        }
    }
    bson_append_document_end(_bson, &child);
    return success;
}

- (BOOL) _appendArray:(NSArray *) array forKey:(NSString *) key error:(NSError **) error {
    bson_raise_if_key_nil_or_too_long();
    BOOL success = YES;
    bson_t child;
    const char *utf8 = [key UTF8String];
    bson_append_array_begin(_bson, utf8, (int)strlen(utf8), &child);
    BSONDocument *childDocument = [[BSONDocument alloc] initWithNativeValue:&child];
    for (NSUInteger i = 0; i < array.count; ++i) {
        id obj = [array objectAtIndex:i];
        NSString *key = [NSString stringWithFormat:@"%lu", (unsigned long)i];
        if (![childDocument appendObject:obj forKey:key error:error]) {
            success = NO;
            break;
        }
    }
    bson_append_array_end(_bson, &child);
    return success;
}

- (BOOL) appendObject:(id) value forKey:(NSString *) key error:(NSError **) error {
    bson_raise_if_nil(value);
    
    // Collection and NSNumber types have their own error handling
    
    if ([value isKindOfClass:[NSDictionary class]])
        return [self _appendDictionary:value forKey:key error:error];
    
    else if ([value isKindOfClass:[NSOrderedSet class]])
        return [self _appendArray:[value array] forKey:key error:error];

    else if ([value isKindOfClass:[NSArray class]])
        return [self _appendArray:value forKey:key error:error];
    
    else if ([value isKindOfClass:[NSNumber class]])
        return [self _appendNumber:value forKey:key error:error];
    
    // Other types

    BOOL result = NO;
    
    if ([NSNull null] == value)
        result = [self appendNullForKey:key];
    
    else if ([BSONUndefined undefined] == value)
        result = [self appendUndefinedForKey:key];
    
    else if ([value isKindOfClass:[BSONObjectID class]])
        result = [self appendObjectID:value forKey:key];
    
    else if ([value isKindOfClass:[BSONRegularExpression class]])
        result = [self appendRegularExpression:value forKey:key];
    
    else if ([value isKindOfClass:[BSONTimestamp class]])
        result = [self appendTimestamp:value forKey:key];
    
    else if ([value isMemberOfClass:[BSONCode class]])
        result = [self appendCode:value forKey:key];
    
    else if ([value isMemberOfClass:[BSONCodeWithScope class]])
        result = [self appendCodeWithScope:value forKey:key];
    
    else if ([value isKindOfClass:[BSONSymbol class]])
        result = [self appendSymbol:value forKey:key];
    
    else if ([value isKindOfClass:[NSString class]])
        result = [self appendString:value forKey:key];
    
    else if ([value isKindOfClass:[NSDate class]])
        result = [self appendDate:value forKey:key];
    
    else if ([value isKindOfClass:[NSData class]])
        result = [self appendData:value forKey:key];
    
    else if ([value isKindOfClass:[BSONDocument class]])
        result = [self appendEmbeddedDocument:value forKey:key];
    
    else {
        if (error) {
            id message =
            [NSString stringWithFormat:@"Unserializable class: %@", NSStringFromClass(([value class]))];
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: message };
            *error = [NSError errorWithDomain:BSONErrorDomain code:BSONSerializationUnsupportedClass userInfo:userInfo];
        }
        return NO;
    }
    
    if (!result) {
        if (error) {
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"BSON object overflow" };
            *error = [NSError errorWithDomain:BSONErrorDomain code:BSONDocumentOverflow userInfo:userInfo];
        }
        return NO;
    }
    
    return result;
}

@end
