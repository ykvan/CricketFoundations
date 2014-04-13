#pragma once
#import <Foundation/Foundation.h>

@protocol BLEDelegate <NSObject>

@optional

- (void)updateConnectionState:(short)state;
- (void)updateLabel:(NSString *)label;
- (void)didReceive:(NSData *)data;

@end
