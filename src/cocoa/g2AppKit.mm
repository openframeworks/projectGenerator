/***************************************************************
 
 GLUI 2 - OpenGL User Interface Library 2
 Copyright 2011 Core S2 - See License.txt for details
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

// All of this should only compile if we are in OSX
#ifdef __APPLE__

#include "g2AppKit.h"
#import <AppKit/AppKit.h>

bool __g2ShowDialog(const char* Message)
{
    // All based on Jorge Arimany Espanque's example code
    // Link: http://jorgearimany.blogspot.com/2010/05/messagebox-from-windows-to-mac.html
    
    // Create core foundation strings
    CFStringRef header_ref = CFStringCreateWithCString( NULL, "Notification", kCFStringEncodingASCII);
    CFStringRef message_ref = CFStringCreateWithCString( NULL, Message, kCFStringEncodingASCII);
    
    // Catch the result of the alert
    CFOptionFlags result;
    
    // Present the notification
    // See full doc at http://developer.apple.com/library/mac/#documentation/
    // CoreFoundation/Reference/CFUserNotificationRef/Reference/reference.html
    CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel, NULL, NULL, NULL, header_ref, message_ref, NULL, NULL, NULL, &result);
    
    // Release the strings
    CFRelease(header_ref);
    CFRelease(message_ref);
    
    // Save the result..
    if(result == kCFUserNotificationDefaultResponse)
        return true;
    else
        return false;
}

bool __g2ShowSaveDialog(const char* Message, const char* FileExtension, char* OutBuffer, int OutLength)
{ 
    // Alloc a temporary ObjC memory manager
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    bool result = false;
    
    // Open panel to open file
    NSSavePanel* spanel = [NSSavePanel savePanel];
    [spanel setTitle:[NSString stringWithCString:Message encoding:NSASCIIStringEncoding]];
    [spanel setAllowedFileTypes:[NSArray arrayWithObjects:[NSString stringWithCString:FileExtension encoding:NSASCIIStringEncoding], nil]];
    NSInteger returnCode = [spanel runModal];
    
    if(returnCode == NSOKButton)
    {
        //NSURL* filenames = [spanel URL];
        //NSString* file = [filenames absoluteString];
        /*if([file length] < OutLength)
        {
            strcpy(OutBuffer, [file cStringUsingEncoding:NSASCIIStringEncoding]);
            result = true;
        }*/
    }
    
    // Else, something failed
    [pool drain];
    return result;
}

bool __g2ShowOpenDialog( char * Path, const char* Message, const char* FileExtension, char* OutBuffer, int OutLength)
{
    // Alloc a temporary ObjC memory manager
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    bool result = false;
    
    // Open panel to open file
    NSOpenPanel* opanel = [NSOpenPanel openPanel];
    [opanel setTitle:[NSString stringWithCString:Message encoding:NSASCIIStringEncoding]];
    [opanel setAllowsMultipleSelection:NO];
    [opanel setCanChooseDirectories:YES];
    [opanel setCanChooseFiles:NO];
    
    
    NSString* string = [NSString stringWithFormat:@"%s" , Path];
    
    [opanel setDirectoryURL:[NSURL fileURLWithPath:string]];
    
    //[opanel setAllowedFileTypes:[NSArray arrayWithObjects:[NSString stringWithCString:FileExtension encoding:NSASCIIStringEncoding], nil]];
    NSInteger returnCode = [opanel runModal];
    
    if(returnCode == NSOKButton)
    {
        
        for (NSURL *url in [opanel URLs]) {
            NSString *urlString = [url path];
            printf("%s\n", [urlString  UTF8String]);
            
            if([urlString length] < OutLength){
                strcpy(OutBuffer, [urlString cStringUsingEncoding:NSASCIIStringEncoding]);
                result = true;
            }
            
            //[input setStringValue:urlString];
            //NSString *myString = [input stringValue];
            //NSString *oldPath = [myString lastPathComponent];
            //[inputDisplay setStringValue:oldPath];
        }
//        //NSArray* filenames = [opanel URLs];
//        
//        
//        NSURL *zUrl = [opanel URL];
//        
//        // read the file
//        NSString * zStr = [NSString stringWithContentsOfURL:zUrl 
//                                                   encoding:NSASCIIStringEncoding 
//                                                      error:NULL];
//        
//        
//        
//        //NSString* file = [NSString stringWithContentsOfURL:[filenames objectAtIndex:0] encoding:NSASCIIStringEncoding error:NULL];
//        
//        printf("file %s", [zStr UTF8String]);
//        
//        
//        if([zStr length] < OutLength)
//        {
//            strcpy(OutBuffer, [zStr cStringUsingEncoding:NSASCIIStringEncoding]);
//            result = true;
//        }
    }
    
    // Else, something failed
    [pool drain];
    return result;
}

// End of apple guard
#endif
