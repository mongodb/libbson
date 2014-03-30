//
//  ObjCBSON.m
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import "ObjCBSON.h"

NSString * const BSONErrorDomain = @"BSONErrorDomain";
//NSInteger const BSONKeyNameErrorStartsWithDollar = 101;
//NSInteger const BSONKeyNameErrorHasDot = 102;
NSInteger const BSONDocumentOverflow = -1;
NSInteger const BSONIntegerOverflow = -2;
NSInteger const BSONIsCorrupt = -3;
NSInteger const BSONSerializationUnsupportedClass = -3;
NSInteger const BSONDocumentInitializationError = -4;
