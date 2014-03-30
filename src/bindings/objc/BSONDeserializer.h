//
//  BSONDeserializer.h
//  libbson
//
//  Created by Paul Melnikow on 3/30/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "bson.h"

@class BSON_OrderedDictionary;

/**
 Private class to handle document serialization
 */
@interface BSONDeserializer : NSObject

+ (NSDictionary *) dictionaryWithNativeDocument:(const bson_t *) bson error:(NSError **) error;

- (id) initWithNativeIterator:(bson_iter_t *) iter;

- (void) appendObject:(id) object forKey:(NSString *) key;
- (void) markCorrupt;
- (void) start;

@property (strong) BSON_OrderedDictionary *dictionary;
@property (assign) BOOL corrupt;
@property (strong) NSError *error;

@end
