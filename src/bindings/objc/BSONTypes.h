//
//  BSONTypes.h
//  ObjCMongoDB
//
//  Copyright 2012 Paul Melnikow and other contributors
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#import <Foundation/Foundation.h>

/**
 Encapsulates an immutable BSON object ID, as a wrapper around a <code>bson_oid_t</code>
 structure.

 Each instance creates a bson_oid_t object during initialization and destoys it on
 deallocation.
 
 @seealso http://www.mongodb.org/display/DOCS/Object+IDs
 */
@interface BSONObjectID : NSObject

/**
 Creates a new, unique object ID.
 */
+ (BSONObjectID *) objectID;

/*
 Creates an object ID from a hexadecimal string.
 @param A 24-character hexadecimal string for the object ID
 @seealso http://www.mongodb.org/display/DOCS/Object+IDs
 */
+ (BSONObjectID *) objectIDWithString:(NSString *) s;

/**
 Creates a object ID from a data representation.
 @param data A 12-byte data representation for the object ID
 @seealso http://www.mongodb.org/display/DOCS/Object+IDs
 */
+ (BSONObjectID *) objectIDWithData:(NSData *) data;

/**
 Returns the 24-digit hexadecimal string value of the receiver.
 @return The hex string value of an object ID.
 */
- (NSString *) stringValue;

/**
 Returns the data representation of the receiver.
 */
//- (NSData *) dataValue;

/**
 Returns the time the object ID was generated.
 */
- (NSDate *) dateGenerated;

/**
 Compares two object ID values.
 */
- (NSComparisonResult)compare:(BSONObjectID *) other;

/**
 Tests for equality with another object ID.
 */
- (BOOL)isEqual:(id)other;

@end

/**
 A wrapper class encapsulating a BSON regular expression.
 */
@interface BSONRegularExpression : NSObject

@property (strong) NSString *pattern;
@property (strong) NSString *options;

@end

/**
 A wrapper and convenience encapsulating a BSON timestamp.
 */
@interface BSONTimestamp : NSObject

@property (assign) uint32_t increment;
@property (assign) uint32_t timeInSeconds;

@end

@class BSONDocument;


/**
 A wrapper class encapsulating a BSON code object.
 */
@interface BSONCode : NSObject
@property (strong) NSString * code;
@end


/**
 A wrapper class encapsulating a BSON code with scope object.
 */
@interface BSONCodeWithScope : BSONCode
@property (strong) BSONDocument * scope;
@end


/**
 A wrapper class encapsulating a BSON symbol object.
 */
@interface BSONSymbol : NSObject
@property (strong) NSString * symbol;
@end


/**
 A wrapper class encapsulating a BSON symbol object.
 */
@interface BSONDatabasePointer : NSObject
@property (strong) NSString * collection;
@property (strong) BSONObjectID * objectID;
@end


/**
 A singleton object encapsulating a BSON minkey object.
 */
@interface BSONMinKey : NSObject
+ (instancetype) minKey;
@end


/**
 A singleton object encapsulating a BSON maxkey object.
 */
@interface BSONMaxKey : NSObject
+ (instancetype) maxKey;
@end


/**
 A singleton object encapsulating a BSON Undefined object.
 */
@interface BSONUndefined : NSObject
+ (instancetype) undefined;
@end
