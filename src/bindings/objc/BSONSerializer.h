//
//  BSONSerializer.h
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "bson.h"

@class BSONDocument;

/**
 Private class to handle document serialization
 */
@interface BSONSerializer : NSObject

+ (instancetype) serializer;
+ (instancetype) serializerWithNativeDocument:(bson_t *) nativeDocument;

- (bson_t *) nativeValue;
- (BOOL) appendObject:(id) value forKey:(NSString *) key error:(NSError **) error;
- (BOOL) serializeDictionary:(NSDictionary *) dictionary error:(NSError **) error;

@property (strong) BSONDocument *document;

@end
