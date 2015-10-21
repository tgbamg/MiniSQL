//
//  CatalogPage.cpp
//  MiniSQL
//
//  Created by zyy on 15/10/18.
//  Copyright © 2015年 Fan Wu. All rights reserved.
//

#include "CatalogPage.hpp"
#include <string>
using namespace std;

void CatalogPage::writeAttr(int start, string cont)
{
    int i,len;
    
    len=(int)cont.length();
    for (i=0; i<len; i++)
        pageData[start+i]=cont[i];
}

string CatalogPage::readAttrName(int num)
{
    int i;
    string s="";
    
    for (i=num*100+2; pageData[i]!=0; i++)
        s = s+pageData[i];
    
    return s;
}

string CatalogPage::readAttrType(int num)
{
    int i;
    string s="";
    
    for (i=num*100+22; pageData[i]!=0; i++)
        s = s+pageData[i];
    
    return s;
}

string CatalogPage::readAttrIndex(int num)
{
    int i;
    string s="";
    
    for (i=num*100+42; pageData[i]!=0; i++)
        s = s+pageData[i];
    
    return s;
}

char CatalogPage::readAttrUnique(int num)
{
    return pageData[num*100+62];
}