//
//  OrderedDictionary.m
//  OrderedDictionary
//
//  Created by Matt Gallagher on 19/12/08.
//  Copyright 2008 Matt Gallagher. All rights reserved.
//
//  MODIFIED -descriptionWithLocale:indent:
//  by Paul Melnikow on March 11, 2012
//
//  MODIFIED variable declarations to use properties
//  by Paul Melnikow on March 29, 2014
//
//  This software is provided 'as-is', without any express or implied
//  warranty. In no event will the authors be held liable for any damages
//  arising from the use of this software. Permission is granted to anyone to
//  use this software for any purpose, including commercial applications, and to
//  alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source
//     distribution.
//

#import "BSON_OrderedDictionary.h"

NSString *DescriptionForObject(NSObject *object, id locale, NSUInteger indent);
NSString *DescriptionForObject(NSObject *object, id locale, NSUInteger indent)
{
	NSString *objectString;
	if ([object isKindOfClass:[NSString class]])
	{
#if __has_feature(objc_arc)
		objectString = (NSString *)object;
#else
		objectString = (NSString *)[[object retain] autorelease];
#endif
	}
	else if ([object respondsToSelector:@selector(descriptionWithLocale:indent:)])
	{
		objectString = [(NSDictionary *)object descriptionWithLocale:locale indent:indent];
	}
	else if ([object respondsToSelector:@selector(descriptionWithLocale:)])
	{
		objectString = [(NSSet *)object descriptionWithLocale:locale];
	}
	else
	{
		objectString = [object description];
	}
	return objectString;
}

@interface BSON_OrderedDictionary ()
@property (strong) NSMutableDictionary *dictionary;
@property (strong) NSMutableArray *array;
@end

@implementation BSON_OrderedDictionary

- (id)init
{
	return [self initWithCapacity:0];
}

- (id)initWithCapacity:(NSUInteger)capacity
{
	self = [super init];
	if (self != nil)
	{
		self.dictionary = [[NSMutableDictionary alloc] initWithCapacity:capacity];
		self.array = [[NSMutableArray alloc] initWithCapacity:capacity];
	}
	return self;
}

- (id)copy
{
	return [self mutableCopy];
}

- (void)setObject:(id)anObject forKey:(id)aKey
{
	if (![self.dictionary objectForKey:aKey])
	{
		[self.array addObject:aKey];
	}
	[self.dictionary setObject:anObject forKey:aKey];
}

- (void)removeObjectForKey:(id)aKey
{
	[self.dictionary removeObjectForKey:aKey];
	[self.array removeObject:aKey];
}

- (NSUInteger)count
{
	return [self.dictionary count];
}

- (id)objectForKey:(id)aKey
{
	return [self.dictionary objectForKey:aKey];
}

- (NSEnumerator *)keyEnumerator
{
	return [self.array objectEnumerator];
}

- (NSEnumerator *)reverseKeyEnumerator
{
	return [self.array reverseObjectEnumerator];
}

- (void)insertObject:(id)anObject forKey:(id)aKey atIndex:(NSUInteger)anIndex
{
	if ([self.dictionary objectForKey:aKey])
	{
		[self removeObjectForKey:aKey];
	}
	[self.array insertObject:aKey atIndex:anIndex];
	[self.dictionary setObject:anObject forKey:aKey];
}

- (id)keyAtIndex:(NSUInteger)anIndex
{
	return [self.array objectAtIndex:anIndex];
}

//
//  MODIFIED by Paul Melnikow on March 11, 2012
//

- (NSString *)descriptionWithLocale:(id)locale indent:(NSUInteger)level
{
	NSMutableString *indentString = [NSMutableString string];
	NSUInteger i, count = level;
	for (i = 0; i < count; i++)
	{
		[indentString appendFormat:@"    "];
	}
	
	NSMutableString *description = [NSMutableString string];
	[description appendFormat:@"%@{", indentString];
	for (NSObject *key in self)
	{
		[description appendFormat:@"\n%@    %@ = %@;",
			indentString,
			DescriptionForObject(key, locale, level+1),
			DescriptionForObject([self objectForKey:key], locale, level+1)];
	}
	[description appendFormat:@"\n%@}", indentString];
	return description;
}

//
//  End MODIFIED by Paul Melnikow on March 11, 2012
//
//  Original below:
//

//- (NSString *)descriptionWithLocale:(id)locale indent:(NSUInteger)level
//{
//	NSMutableString *indentString = [NSMutableString string];
//	NSUInteger i, count = level;
//	for (i = 0; i < count; i++)
//	{
//		[indentString appendFormat:@"    "];
//	}
//	
//	NSMutableString *description = [NSMutableString string];
//	[description appendFormat:@"%@{\n", indentString];
//	for (NSObject *key in self)
//	{
//		[description appendFormat:@"%@    %@ = %@;\n",
//			indentString,
//			DescriptionForObject(key, locale, level),
//			DescriptionForObject([self objectForKey:key], locale, level)];
//	}
//	[description appendFormat:@"%@}\n", indentString];
//	return description;
//}

@end
