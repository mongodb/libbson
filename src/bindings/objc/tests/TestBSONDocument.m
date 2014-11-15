//
//  TestBSONDocument.m
//  libbson
//
//  Created by Paul Melnikow on 3/29/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "BSONDocument.h"
#import "BSONTypes.h"

@interface TestBSONDocument : XCTestCase

@end

@implementation TestBSONDocument

- (void) testAppendObjCTypes {
    BSONDocument *document = [BSONDocument document];
    
    // NSData
    XCTAssertThrows([document appendData:nil forKey:@"testKey"]);
    XCTAssertNoThrow([document appendData:[NSData data] forKey:@"testKey"]);

    XCTAssertNoThrow([document appendBool:YES forKey:@"testKey"]);
    XCTAssertNoThrow([document appendBool:NO forKey:@"testKey"]);

    XCTAssertNoThrow([document appendDouble:0 forKey:@"testKey"]);
    XCTAssertNoThrow([document appendDouble:2.5f forKey:@"testKey"]);

    XCTAssertNoThrow([document appendInt32:0 forKey:@"testKey"]);
    XCTAssertNoThrow([document appendInt32:42 forKey:@"testKey"]);

    XCTAssertNoThrow([document appendInt64:0 forKey:@"testKey"]);
    XCTAssertNoThrow([document appendInt64:42 forKey:@"testKey"]);

    XCTAssertThrows([document appendString:nil forKey:@"testKey"]);
    XCTAssertNoThrow([document appendString:@"" forKey:@"testKey"]);
    XCTAssertNoThrow([document appendString:@"test value" forKey:@"testKey"]);

    XCTAssertThrows([document appendDate:nil forKey:@"testKey"]);
    XCTAssertNoThrow([document appendDate:[NSDate date] forKey:@"testKey"]);
}

- (void) testAppendBSONTypes {
    BSONDocument *document = [BSONDocument document];
    
    // BSONRegularExpression
    BSONRegularExpression *regex = nil;
    XCTAssertThrows([document appendRegularExpression:regex forKey:@"testKey"]);
    
    regex = [[BSONRegularExpression alloc] init];
    XCTAssertThrows([document appendRegularExpression:regex forKey:@"testKey"]);

    regex = [[BSONRegularExpression alloc] init];
    regex.options = @"i";
    XCTAssertThrows([document appendRegularExpression:regex forKey:@"testKey"]);
    
    regex = [[BSONRegularExpression alloc] init];
    regex.pattern = @"text";
    XCTAssertNoThrow([document appendRegularExpression:regex forKey:@"testKey"]);
    
    // BSONCode
    BSONCode *code = nil;
    XCTAssertThrows([document appendCode:code forKey:@"testKey"]);
    
    code = [[BSONCode alloc] init];
    XCTAssertThrows([document appendCode:code forKey:@"testKey"]);
    
    code = [[BSONCode alloc] init];
    code.code = @"here's some code";
    XCTAssertNoThrow([document appendCode:code forKey:@"testKey"]);
    
    // BSONCodeWithScope
    BSONCodeWithScope *codeWithScope = nil;
    XCTAssertThrows([document appendCodeWithScope:codeWithScope forKey:@"testKey"]);
    
    codeWithScope = [[BSONCodeWithScope alloc] init];
    XCTAssertThrows([document appendCodeWithScope:codeWithScope forKey:@"testKey"]);

    codeWithScope = [[BSONCodeWithScope alloc] init];
    codeWithScope.scope = [BSONDocument document];
    XCTAssertThrows([document appendCodeWithScope:codeWithScope forKey:@"testKey"]);

    codeWithScope = [[BSONCodeWithScope alloc] init];
    codeWithScope.code = @"here's some code";
    XCTAssertThrows([document appendCodeWithScope:codeWithScope forKey:@"testKey"]);
    
    codeWithScope = [[BSONCodeWithScope alloc] init];
    codeWithScope.code = @"here's some code";
    codeWithScope.scope = [BSONDocument document];
    XCTAssertNoThrow([document appendCodeWithScope:codeWithScope forKey:@"testKey"]);

    // BSONDatabasePointer
    BSONDatabasePointer *dbPointer = nil;
    XCTAssertThrows([document appendDatabasePointer:dbPointer forKey:@"testKey"]);
    
    dbPointer = [[BSONDatabasePointer alloc] init];
    XCTAssertThrows([document appendDatabasePointer:dbPointer forKey:@"testKey"]);

    dbPointer = [[BSONDatabasePointer alloc] init];
    dbPointer.collection = @"ponies";
    XCTAssertThrows([document appendDatabasePointer:dbPointer forKey:@"testKey"]);

    dbPointer = [[BSONDatabasePointer alloc] init];
    dbPointer.objectID = [BSONObjectID objectID];
    XCTAssertThrows([document appendDatabasePointer:dbPointer forKey:@"testKey"]);

    dbPointer = [[BSONDatabasePointer alloc] init];
    dbPointer.collection = @"ponies";
    dbPointer.objectID = [BSONObjectID objectID];
    XCTAssertNoThrow([document appendDatabasePointer:dbPointer forKey:@"testKey"]);

    // BSONObjectID
    BSONObjectID *objectID = nil;
    XCTAssertThrows([document appendObjectID:objectID forKey:@"testKey"]);

    objectID = [BSONObjectID objectID];
    XCTAssertNoThrow([document appendObjectID:objectID forKey:@"testKey"]);
    
    // BSONSymbol
    BSONSymbol *symbol = nil;
    XCTAssertThrows([document appendSymbol:symbol forKey:@"testKey"]);

    symbol = [[BSONSymbol alloc] init];
    XCTAssertThrows([document appendSymbol:symbol forKey:@"testKey"]);

    symbol = [[BSONSymbol alloc] init];
    symbol.symbol = @"a symbol";
    XCTAssertNoThrow([document appendSymbol:symbol forKey:@"testKey"]);
    
    // BSONTimestamp
    BSONTimestamp *timestamp = nil;
    XCTAssertThrows([document appendTimestamp:timestamp forKey:@"testKey"]);
    
    timestamp = [[BSONTimestamp alloc] init];
    XCTAssertNoThrow([document appendTimestamp:timestamp forKey:@"testKey"]);
    
    // minkey, maxkey, null, undefined
    XCTAssertNoThrow([document appendMinKeyForKey:@"testKey"]);
    XCTAssertNoThrow([document appendMaxKeyForKey:@"testKey"]);
    XCTAssertNoThrow([document appendNullForKey:@"testKey"]);
    XCTAssertNoThrow([document appendUndefinedForKey:@"testKey"]);
    
    // Embedded document
    XCTAssertThrows([document appendEmbeddedDocument:nil forKey:@"testKey"]);
    XCTAssertNoThrow([document appendEmbeddedDocument:[BSONDocument document] forKey:@"testKey"]);
}

- (void) testAppendWrongObjectTypes {
    BSONDocument *document = [BSONDocument document];
    
    id number = @42;
    XCTAssertThrows([document appendString:number forKey:@"testKey"]);

    id str = @"a string";
    XCTAssertThrows([document appendData:str forKey:@"testKey"]);
    XCTAssertThrows([document appendCode:str forKey:@"testKey"]);
    XCTAssertThrows([document appendCodeWithScope:str forKey:@"testKey"]);
    XCTAssertThrows([document appendDatabasePointer:str forKey:@"testKey"]);
    XCTAssertThrows([document appendEmbeddedDocument:str forKey:@"testKey"]);
    XCTAssertThrows([document appendObjectID:str forKey:@"testKey"]);
    XCTAssertThrows([document appendRegularExpression:str forKey:@"testKey"]);
    XCTAssertThrows([document appendSymbol:str forKey:@"testKey"]);
    XCTAssertThrows([document appendDate:str forKey:@"testKey"]);
    XCTAssertThrows([document appendTimestamp:str forKey:@"testKey"]);
}

- (void) testIsEmpty {
    BSONDocument *document = [BSONDocument document];
    XCTAssertTrue(document.isEmpty);

    document = [BSONDocument document];
    [document appendString:@"test string" forKey:@"testKey"];
    XCTAssertFalse(document.isEmpty);
}

- (void) testCompare {
    BSONDocument *documentA1 = [BSONDocument document];
    [documentA1 appendString:@"a" forKey:@"testKey"];
    BSONDocument *documentA2 = [BSONDocument document];
    [documentA2 appendString:@"a" forKey:@"testKey"];
    BSONDocument *documentB = [BSONDocument document];
    [documentB appendString:@"b" forKey:@"testKey"];
    
    XCTAssertEqual([documentA1 compare:documentA2], NSOrderedSame);
    XCTAssertEqual([documentA2 compare:documentA1], NSOrderedSame);
    XCTAssertEqual([documentA1 compare:documentB], NSOrderedAscending);
    XCTAssertEqual([documentB compare:documentA1], NSOrderedDescending);    
}

- (void) testHasField {
    BSONDocument *document = [BSONDocument document];
    XCTAssertFalse([document hasField:@"testKey"]);
    XCTAssertFalse([document hasField:@"bogusKey"]);
    [document appendString:@"a" forKey:@"testKey"];
    XCTAssertTrue([document hasField:@"testKey"]);
    XCTAssertFalse([document hasField:@"bogusKey"]);
}

@end
