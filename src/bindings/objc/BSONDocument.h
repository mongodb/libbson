//
//  BSONDocument.h
//  libbson
//
//  Created by Paul Melnikow on 3/28/14.
//  Copyright (c) 2014 Paul Melnikow. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BSONTypes.h"

@interface BSONDocument : NSObject


/**
 Creates an empty BSON document.
 */
+ (instancetype) document;


/**
 Creates an empty BSON document with the given capacity, which must be
 smaller than INT_MAX.
 */
+ (instancetype) documentWithCapacity:(NSUInteger) bytes;


/**
 Returns a BSON document created using the given data block, which it
 retains and accesses directly.
 
 If <i>data</i> is mutable, it retains a copy instead.
 
 @param data An instance of <code>NSData</code> with the binary data for the new
 document.
 */
+ (instancetype) documentWithData:(NSData *) data;
+ (instancetype) documentWithData:(NSData *) data noCopy:(BOOL) noCopy;

- (NSString *) description;

/**
 Returns an immutable <code>NSData</code> object with a copy of the document's
 BSON data buffer.
 
 The NSData object is guaranteed to remain valid even if the receiver is deallocated.
 
 @returns An immutable <code>NSData</code> object pointing to the BSON data buffer.
 */
- (NSData *) dataValue;

- (NSData *) dataValueNoCopy;

- (instancetype) copy;
- (void) reinit;

- (BOOL) isEmpty;
- (BOOL) hasField:(NSString *) key;
- (NSComparisonResult) compare:(BSONDocument *) other;
- (BOOL) isEqual:(BSONDocument *) document;

- (BOOL) appendData:(NSData *) value forKey:(NSString *) key;
- (BOOL) appendBool:(BOOL) value forKey:(NSString *) key;
- (BOOL) appendCode:(BSONCode *) value forKey:(NSString *) key;
- (BOOL) appendCodeWithScope:(BSONCodeWithScope *) value forKey:(NSString *) key;
- (BOOL) appendDatabasePointer:(BSONDatabasePointer *) value forKey:(NSString *) key;
- (BOOL) appendDouble:(double) value forKey:(NSString *) key;
- (BOOL) appendEmbeddedDocument:(BSONDocument *) value forKey:(NSString *) key;
- (BOOL) appendInt32:(int32_t) value forKey:(NSString *) key;
- (BOOL) appendInt64:(int64_t) value forKey:(NSString *) key;
- (BOOL) appendMinKeyForKey:(NSString *) key;
- (BOOL) appendMaxKeyForKey:(NSString *) key;
- (BOOL) appendNullForKey:(NSString *) key;
- (BOOL) appendObjectID:(BSONObjectID *) value forKey:(NSString *) key;
- (BOOL) appendRegularExpression:(BSONRegularExpression *) value forKey:(NSString *) key;
- (BOOL) appendString:(NSString *) value forKey:(NSString *) key;
- (BOOL) appendSymbol:(BSONSymbol *) value forKey:(NSString *) key;
- (BOOL) appendDate:(NSDate *) value forKey:(NSString *) key;
- (BOOL) appendTimestamp:(BSONTimestamp *) value forKey:(NSString *) key;
- (BOOL) appendUndefinedForKey:(NSString *) key;

- (BOOL) appendObject:(id) value forKey:(NSString *) key error:(NSError **) error;

+ (NSUInteger) maximumCapacity;

@end
