//
//  ObjCBSON.h
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSString * const BSONErrorDomain;
FOUNDATION_EXPORT NSInteger const BSONKeyNameErrorStartsWithDollar;
FOUNDATION_EXPORT NSInteger const BSONKeyNameErrorHasDot;
FOUNDATION_EXPORT NSInteger const BSONDocumentOverflow;
FOUNDATION_EXPORT NSInteger const BSONIntegerOverflow;
FOUNDATION_EXPORT NSInteger const BSONIsCorrupt;
FOUNDATION_EXPORT NSInteger const BSONSerializationUnsupportedClass;
FOUNDATION_EXPORT NSInteger const BSONDocumentInitializationError;
