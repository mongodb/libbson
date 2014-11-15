//
//  BSONVisitor.h
//  libbson
//
//  Created by Paul Melnikow on 3/29/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <Foundation/Foundation.h>

@class BSONDocument;

@interface BSONSerialization : NSObject

+ (NSDictionary *) dictionaryWithBSONData:(NSData *) data error:(NSError **) error;
+ (NSData *) BSONDataWithDictionary:(NSDictionary *) dictionary error:(NSError **) error;

@end
