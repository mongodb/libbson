//
//  BSONTypes.m
//  ObjCMongoDB
//
//  Copyright 2012 Paul Melnikow and other contributors
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#import "BSONTypes.h"
#import "BSON_Helper.h"
#import "bson.h"


@interface BSONObjectID ()
@property (retain) NSString *privateStringValue;
@end

@interface BSONObjectID (Module)
- (id) initWithNativeValue:(const bson_oid_t *) objectIDPointer;
@end

@implementation BSONObjectID {
    bson_oid_t _oid;
}

#pragma mark - Initialization

- (id) init {
    if (self = [super init]) {
        bson_oid_init(&_oid, NULL);
    }
    return self;
}

- (id) initWithString:(NSString *) s {
    const char * utf8 = s.UTF8String;
    if (!bson_oid_is_valid(utf8, strlen(utf8))) {
        [NSException raise:NSInvalidArgumentException format:@"String is not a valid OID"];
    }
    if (self = [super init]) {
        bson_oid_init_from_string(&_oid, utf8);
    }
    return self;
}

- (id) initWithData:(NSData *) data {
    if ((self = [super init])) {
        if ([data length] != 12) return self = nil;
        bson_oid_init_from_data(&_oid, data.bytes);
    }
    return self;
}

- (id) initWithNativeValue:(const bson_oid_t *) objectIDPointer {
    if (self = [super init]) {
        _oid = *objectIDPointer;
    }
    return self;
}

+ (BSONObjectID *) objectID {
    return [[self alloc] init];
}

+ (BSONObjectID *) objectIDWithString:(NSString *) s {
    return [[self alloc] initWithString:s];
}

+ (BSONObjectID *) objectIDWithData:(NSData *) data {
    return [[self alloc] initWithData:data];
}

- (id) copyWithZone:(NSZone *) zone {
	return [[BSONObjectID allocWithZone:zone] initWithNativeValue:&_oid];
}

- (bson_oid_t) nativeValue { return _oid; }

- (NSDate *) dateGenerated {
    return [NSDate dateWithTimeIntervalSince1970:bson_oid_get_time_t(&_oid)];
}

- (NSUInteger) hash {
	return bson_oid_hash(&_oid);
}

- (NSString *) description {
    return [self stringValue];
}

- (NSString *) stringValue {
    if (self.privateStringValue) return self.privateStringValue;
    // str must be at least 24 hex chars + null byte
    char buffer[25];
    bson_oid_to_string(&_oid, buffer);
    return self.privateStringValue = [NSString stringWithUTF8String:buffer];
}

- (NSComparisonResult) compare:(BSONObjectID *) other {
    if (other == nil) [NSException raise:NSInvalidArgumentException format:@"Nil argument"];
    bson_oid_t otherNative = other.nativeValue;
    return bson_NSComparisonResultFromQsort(bson_oid_compare(&_oid, &otherNative));
}

- (BOOL) isEqual:(BSONObjectID *) other {
    if (![other isKindOfClass:[BSONObjectID class]]) return NO;
    bson_oid_t otherNative = other.nativeValue;
    return bson_oid_equal(&_oid, &otherNative);
}

@end


@implementation BSONRegularExpression
@end


@implementation BSONTimestamp
@end


@implementation BSONCode
@end


@implementation BSONCodeWithScope
@end


@implementation BSONSymbol
@end


@implementation BSONMinKey

+ (instancetype) minKey {
    static BSONMinKey *singleton;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        singleton = [[BSONMinKey alloc] init];
    });
    return singleton;
}

@end


@implementation BSONMaxKey

+ (instancetype) maxKey {
    static BSONMaxKey *singleton;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        singleton = [[BSONMaxKey alloc] init];
    });
    return singleton;
}

@end


@implementation BSONDatabasePointer
@end


@implementation BSONUndefined

+ (instancetype) undefined {
    static BSONUndefined *singleton;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        singleton = [[BSONUndefined alloc] init];
    });
    return singleton;
}

@end

