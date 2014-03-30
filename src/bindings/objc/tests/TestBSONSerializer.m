//
//  TestBSONSerializer.m
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "BSONSerializer.h"

@interface TestBSONSerializer : XCTestCase

@end

@implementation TestBSONSerializer

- (void) testAppendObject {
    BSONSerializer *serializer = [BSONSerializer serializer];
    NSError *error = nil;
    
    XCTAssertTrue([serializer appendObject:@"test string" forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    XCTAssertTrue([serializer appendObject:[NSDate date] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    XCTAssertTrue([serializer appendObject:[NSData data] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    
    XCTAssertTrue([serializer appendObject:[NSNumber numberWithBool:YES] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    XCTAssertTrue([serializer appendObject:[NSNumber numberWithDouble:2.5] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    XCTAssertTrue([serializer appendObject:[NSNumber numberWithInt:42] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
    XCTAssertTrue([serializer appendObject:[NSNumber numberWithInteger:42] forKey:@"testKey" error:&error]);
    XCTAssertNil(error);
}

@end
