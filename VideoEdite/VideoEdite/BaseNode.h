//
//  BaseNode.h
//  VideoEdite
//
//  Created by ronglei on 14-6-14.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#ifndef __VideoEdite__BaseNode__
#define __VideoEdite__BaseNode__

#include <iostream>

typedef enum NodeType{
    videoType,
    audioTyep
}NoteType;

#endif /* defined(__VideoEdite__BaseNode__) */

class BaseNode{
public:
    
private:
    NodeType m_type;
    
};