//
//  BSONVisitor.m
//  libbson
//
//  Created by Paul Melnikow on 3/29/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import "BSONSerialization.h"
#import "BSONSerializer.h"
#import "BSONDeserializer.h"
#import "BSON_OrderedDictionary.h"
#import "BSONTypes.h"
#import "BSON_Helper.h"
#import "BSONDocument.h"
#import "ObjCBSON.h"
#import <bson.h>

#pragma mark - Module interfaces

@interface BSONDocument (Module)
- (const bson_t *) nativeValue;
- (id) initWithNativeValue:(bson_t *) bson destroyOnDealloc:(BOOL) destroyOnDealloc;
@end

@interface BSONSerialization (Module)
+ (NSDictionary *) dictionaryWithDocument:(BSONDocument *) document error:(NSError **) error;
+ (BSONDocument *) BSONDocumentWithDictionary:(NSDictionary *) dictionary error:(NSError **) error;
@end

@implementation BSONSerialization

+ (NSDictionary *) dictionaryWithDocument:(BSONDocument *) document error:(NSError **) error {
    return [BSONDeserializer dictionaryWithNativeDocument:document.nativeValue error:error];
}

+ (NSDictionary *) dictionaryWithBSONData:(NSData *) data error:(NSError **) error {
    bson_raise_if_nil(data);
    BSONDocument *document = [BSONDocument documentWithData:data noCopy:YES];
    if (document == nil) {
        if (error) {
            NSDictionary *userInfo = @{ NSLocalizedDescriptionKey: @"Unable to create document" };
            *error = [NSError errorWithDomain:BSONErrorDomain code:-1 userInfo:userInfo];
        }
        return nil;
    }
    return [self dictionaryWithDocument:document error:error];
}

+ (BSONDocument *) BSONDocumentWithDictionary:(NSDictionary *) dictionary error:(NSError **) error {
    bson_raise_if_nil(dictionary);
    BSONSerializer *serializer = [BSONSerializer serializer];
    if ([serializer serializeDictionary:dictionary error:error])
        return serializer.document;
    else
        return nil;
}

+ (NSData *) BSONDataWithDictionary:(NSDictionary *) dictionary error:(NSError **) error {
    return [[self BSONDocumentWithDictionary:dictionary error:error] dataValue];
}

@end
