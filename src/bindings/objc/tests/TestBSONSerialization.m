//
//  libbson_tests.m
//  libbson tests
//
//  Created by Paul Melnikow on 3/28/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "BSONSerialization.h"

@interface TestBSONSerialization : XCTestCase

@end

@implementation TestBSONSerialization

- (void) testSimpleExample {
    NSDictionary *sample =
  @{
    @"one": @1,
    @"two": @(2.0f),
    @"three": @"3",
    @"five": @YES,
    };
    
    NSError *error = nil;
    NSData *data = [BSONSerialization BSONDataWithDictionary:sample error:&error];
    XCTAssertNotNil(data);
    XCTAssertNil(error);
    
    NSDictionary *result = [BSONSerialization dictionaryWithBSONData:data error:&error];
    XCTAssertNotNil(result);
    XCTAssertNil(error);

    XCTAssertEqualObjects(result, sample);
}

- (void) testArrayExample {
    NSDictionary *sample =
    @{
      @"four": @[ @"zero", @"one", @"two", @"three" ],
      };
    
    NSError *error = nil;
    NSData *data = [BSONSerialization BSONDataWithDictionary:sample error:&error];
    XCTAssertNotNil(data);
    XCTAssertNil(error);
    
    NSDictionary *result = [BSONSerialization dictionaryWithBSONData:data error:&error];
    XCTAssertNotNil(result);
    XCTAssertNil(error);
    
    XCTAssertEqualObjects(result, sample);
}

- (NSDictionary *) serializeAndDeserialize:(NSDictionary *) dictionary {
    NSError *error = nil;
    NSData *data = [BSONSerialization BSONDataWithDictionary:dictionary error:&error];
    if (!data) return nil;
    
    NSDictionary *result = [BSONSerialization dictionaryWithBSONData:data error:&error];
    if (!result) return nil;
    
    return result;
}

- (void) testBuiltinTypes {
    id original = nil;
    id result = nil;
    NSError *error = nil;
    
    original = @"test string";
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);

    // +[NSNumber numberWithBool:]
    original = @YES;
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"c"); // "c" == char
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) result objCType]], @"c"); // "c" == char

    // Core Foundation boolean
    original = (id)kCFBooleanTrue;
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"c"); // "c" == char
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) result objCType]], @"c"); // "c" == char
    
    // +[NSNumber numberWithChar:]
    original = [NSNumber numberWithChar:-42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"c"); // "c" == char
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithChar:CHAR_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"c"); // "c" == char
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithChar:CHAR_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"c"); // "c" == char
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    // +[NSNumber numberWithDouble:]
    original = [NSNumber numberWithDouble:-42.5];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"d"); // "d" == double
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    original = [NSNumber numberWithDouble:DBL_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"d"); // "d" == double
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    original = [NSNumber numberWithDouble:DBL_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"d"); // "d" == double
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    // +[NSNumber numberWithFloat:]
    original = [NSNumber numberWithFloat:-42.5];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"f"); // "f" == float
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    original = [NSNumber numberWithFloat:FLT_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"f"); // "f" == float
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    original = [NSNumber numberWithFloat:FLT_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"f"); // "f" == float
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "d")); // "d" == double

    // +[NSNumber numberWithInt:]
    original = [NSNumber numberWithInt:-42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"i"); // "i" == int
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithInt:INT_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"i"); // "i" == int
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithInt:INT_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"i"); // "i" == int
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    // +[NSNumber numberWithInteger:]
    original = [NSNumber numberWithInteger:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithInteger:NSIntegerMax];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithInteger:NSIntegerMin];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithLong:]
    original = [NSNumber numberWithLong:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithLong:LONG_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithLong:LONG_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long
    
    // +[NSNumber numberWithLongLong:]
    original = [NSNumber numberWithLongLong:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithLongLong:LONG_LONG_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithLongLong:LONG_LONG_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithShort:]
    original = [NSNumber numberWithShort:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"s"); // "s" == short
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithShort:SHRT_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"s"); // "s" == short
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    original = [NSNumber numberWithShort:SHRT_MIN];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"s"); // "s" == short
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int

    // +[NSNumber numberWithUnsignedChar:]
    original = [NSNumber numberWithUnsignedChar:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"s"); // "s" == short
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "i")); // "i" == int
    
    original = [NSNumber numberWithUnsignedInt:UCHAR_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedInt:0];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithUnsignedInt:]
    original = [NSNumber numberWithUnsignedInt:-42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedInt:UINT_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedInt:0];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithUnsignedInteger:]
    original = [NSNumber numberWithUnsignedInteger:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedInteger:NSIntegerMax];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedInteger:NSIntegerMax + 1];
    XCTAssertNil([BSONSerialization BSONDataWithDictionary:@{ @"testKey": original } error:&error]);
    XCTAssertEqualObjects([error localizedDescription], @"Unsigned integer value overflows BSON signed integer capacity");

    original = [NSNumber numberWithUnsignedInteger:0];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "q" == long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long
    
    // +[NSNumber numberWithUnsignedLong:]
    original = [NSNumber numberWithUnsignedLong:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedLong:LONG_MAX];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long
    
    original = [NSNumber numberWithUnsignedLong:LONG_MAX + 1];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"Q"); // "Q" == unsigned long
    XCTAssertNil([BSONSerialization BSONDataWithDictionary:@{ @"testKey": original } error:&error]);
    XCTAssertEqualObjects([error localizedDescription], @"Unsigned integer value overflows BSON signed integer capacity");

    original = [NSNumber numberWithUnsignedLong:0];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithUnsignedLongLong:]
    original = [NSNumber numberWithUnsignedLongLong:42];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedLongLong:LONG_LONG_MAX];
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    original = [NSNumber numberWithUnsignedLongLong:LONG_LONG_MAX+1];
    XCTAssertNil([BSONSerialization BSONDataWithDictionary:@{ @"testKey": original } error:&error]);
    XCTAssertEqualObjects([error localizedDescription], @"Unsigned integer value overflows BSON signed integer capacity");
    
    original = [NSNumber numberWithUnsignedLongLong:0];
    XCTAssertEqualObjects([NSString stringWithUTF8String:[(NSNumber *) original objCType]], @"q"); // "Q" == unsigned long
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    XCTAssertTrue(0 == strcmp([(NSNumber *) result objCType], "q")); // "q" == long

    // +[NSNumber numberWithUnsignedShort:]
    original = [NSNumber numberWithUnsignedShort:42];
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    
    original = [NSNumber numberWithUnsignedShort:USHRT_MAX];
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
    
    original = [NSNumber numberWithUnsignedShort:0];
    result = [[self serializeAndDeserialize:@{ @"testKey": original }] objectForKey:@"testKey"];
    XCTAssertEqualObjects(result, original);
}

- (void) testEmbeddedObjectExample {
    NSDictionary *sample =
    @{
      @"four": @{
              @"one": @1,
              @"two": @(2.0f),
              @"three": @"3",
              @"five": @YES,
              }
    };
    
    NSError *error = nil;
    NSData *data = [BSONSerialization BSONDataWithDictionary:sample error:&error];
    XCTAssertNotNil(data);
    XCTAssertNil(error);
    
    NSDictionary *result = [BSONSerialization dictionaryWithBSONData:data error:&error];
    XCTAssertNotNil(result);
    XCTAssertNil(error);
    
    XCTAssertEqualObjects(result, sample);
}

@end
