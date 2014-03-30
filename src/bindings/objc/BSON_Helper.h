//
//  BSON_Helper.h
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

#define bson_raise_if_string_too_long(str, message) \
    if (str.length > INT_MAX) [NSException raise:NSInvalidArgumentException format:message];

#define bson_raise_if_key_nil_or_too_long() \
    if (key == nil) [NSException raise:NSInvalidArgumentException format:@"Nil parameter"]; \
    bson_raise_if_string_too_long(key, @"Key is too long")

#define bson_raise_if_nil(obj) \
    if (obj == nil) [NSException raise:NSInvalidArgumentException format:@"Nil parameter"];

#define bson_raise_if_value_is_not_instance_of_class(cls) \
    if (![value isKindOfClass:[cls class]]) \
        [NSException raise:NSInvalidArgumentException format:@"Value is the wrong type"];

static inline NSComparisonResult bson_NSComparisonResultFromQsort (int result) {
    if (result < 0)
        return NSOrderedAscending;
    else if (result == 0)
        return NSOrderedSame;
    else
        return NSOrderedDescending;
}
