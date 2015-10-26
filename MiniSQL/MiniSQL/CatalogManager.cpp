//
//  CatalogManager.cpp
//  MiniSQL
//
//  Created by zyy on 15/10/17.
//  Copyright © 2015年 Fan Wu. All rights reserved.
//

#include "CatalogManager.hpp"
#include "BufferManager.hpp"
#include "Page.hpp"
#include "CatalogPage.hpp"
#include "IndexCatalogPage.hpp"
#include "TableInfo.hpp"
#include <stdio.h>
#include <string>
using namespace std;

void CatalogManager::insertTable(TableInfo table)
{
    CatalogPage page;
    BufferManager buffer;
    int i;
    string s;
    
    if (tableExisted(table.tableName))
    {
        printf("Insert table %s failed. Table already existed.\n",table.tableName.c_str());
        return;
    }
    
    page.tableName = table.tableName;
    page.pageData[0] = (char)table.attrNum;
    page.pageData[1] = (char)table.primaryKeyLoc;
    for (i=0; i<table.attrNum; i++)
    {
        page.writeAttr(i*100+2, table.attrName[i]);
        page.writeAttr(i*100+22, table.attrType[i]);
        page.writeAttr(i*100+42, table.attrIndex[i]);
        page.pageData[i*100+62] = table.attrUnique[i];
        *(int*)(page.pageData+(i*100+82)) = 0;      //该属性上的索引总数，初始为0
    }
    
    buffer.writePage(page);
    insertIndex(table.tableName, table.attrName[table.primaryKeyLoc], table.tableName+table.attrName[table.primaryKeyLoc]);
}

void CatalogManager::dropTable(string tableName)
{
    if (!tableExisted(tableName))
    {
        printf("Table does not exist!\n");
    }
    else
    {
        BufferManager buffer;
        IndexCatalogPage indexPage;
        int n,nn,i,x,k;
        string s;
        
        indexPage.pageIndex=1;
        buffer.readPage(indexPage);
        n=*(int*)indexPage.pageData;
        nn=n;
        i=1;
        k=0;
        while (i<=n)                                //开始逐条检查，删除这张表上所有的索引
        {
            x=indexPage.readPrevDel(i);
            if (x==0)                               //如果当前条未被删除
            {
                s=indexPage.readTableName(i);
                if (s==tableName)                   //如果是这张表的索引，删掉
                {
                    k++;                            //k用来记录删除了多少条索引
                    indexPage.deleteIndex(i);
                    //此处应有个api接口，用来真正删除索引
                }
            }
            else                                    //如果当前条已被删除，则最后一条位置后移
                n++;
            
            i++;
        }
        
        indexPage.pageIndex=1;                      //记录索引信息首页的索引总数也需要改
        buffer.readPage(indexPage);
        *(int*)indexPage.pageData=nn-k;
        buffer.writePage(indexPage);
        buffer.deleteTableCatalogFile(tableName);   //删除当前表的文件
    }
}

bool CatalogManager::tableExisted(string tableName)
{
    BufferManager buffer;
    
    if (buffer.tableCatalogFileIsExist(tableName))
        return 1;
    else
        return 0;
}

bool CatalogManager::attrExisted(string tableName, string attrName)
{
    if (!tableExisted(tableName))
    {
        printf("Failed to find Attribution %s on Table %s. Table %s does not existed.", attrName.c_str(),tableName.c_str(),tableName.c_str());
        
        return 0;
    }
    else
    {
        CatalogPage page;
        BufferManager buffer;
        int num,i;
        
        page.tableName = tableName;
        buffer.readPage(page);
        num = (int)page.pageData[0];
        for (i=0; i<num; i++)
            if (page.readAttrName(i) == attrName)
                return 1;

        return 0;
    }
}

int CatalogManager::attrType(string tableName, string attrName)
{
    int i,j,num;
    string s="";
    
    CatalogPage page;
    BufferManager buffer;
    
    page.tableName = tableName;
    buffer.readPage(page);
    num = (int)page.pageData[0];
    for (i=0; i<num; i++)
        if (page.readAttrName(i) == attrName)
            s = page.readAttrType(i);
    
    if (s == "")
        return 0;
    else
    if (s == "int")
        return 1;
    else
    if (s == "float")
        return 2;
    else
    {
        j=0;
        for (i=5; s[i]!=')'; i++)
            j = j*10+s[i]-'0';
        
        return j+2;
    }
}

bool CatalogManager::attrUnique(string tableName, string attrName)
{
    int i,num;
    char c = '\0';
    CatalogPage page;
    BufferManager buffer;
    
    page.tableName = tableName;
    buffer.readPage(page);
    num = (int)page.pageData[0];
    for (i=0; i<num; i++)
        if (page.readAttrName(i) == attrName)
            c = page.readAttrUnique(i);

    if (c=='Y')
        return 1;
    else
        return 0;
}

string CatalogManager::primaryKey(string tableName)
{
    CatalogPage page;
    BufferManager buffer;
    
    page.tableName = tableName;
    buffer.readPage(page);
    return page.readAttrName((int)page.pageData[1]);
}

bool CatalogManager::indexExisted(string indexName)
{
    string s;
    
    s=indexLocation(indexName);
    if (s=="")
        return 0;
    else
    {
        printf("Index name existed! The table is on %s.\n",s.c_str());
        return 1;
    }
}

string CatalogManager::indexLocation(string indexName)
{
    BufferManager buffer;
    IndexCatalogPage indexPage;
    int n,i,k,x;
    string s,ans;
    
    ans="";
    indexPage.pageIndex=1;
    buffer.readPage(indexPage);
    n=*(int*)indexPage.pageData;
    i=1;
    k=0;
    while (i<=n)                                //开始逐条检查，看看这个索引名有没有
    {
        x=indexPage.readPrevDel(i);
        if (x==0)                               //如果当前条未被删除
        {
            s=indexPage.readIndexName(i);
            if (s==indexName)                   //如果找到了
            {
                ans=indexPage.readTableName(i)+" "+indexPage.readAttrName(i);
                return ans;
            }
        }
        else                                    //如果当前条已被删除，则最后一条位置后移
            n++;
        
        i++;
    }

    return "";
}

void CatalogManager::insertIndex(string tableName, string attrName, string indexName)
{
    IndexCatalogPage indexPage;
    if (indexExisted(indexName))
    {
        printf("Insert index %s failed. Index name already existed!\n", indexName.c_str());
    }
    else
    if (!tableExisted(tableName))
    {
        printf("Insert index %s failed. Table %s does not exist!\n", indexName.c_str(),tableName.c_str());
    }
    else
    if (!attrExisted(tableName, attrName))
    {
        printf("Insert index %s failed. Attribution %s on Table %s does not exist!\n", indexName.c_str(), attrName.c_str(),tableName.c_str());
    }
    else
    if (!attrUnique(tableName, attrName))
    {
        printf("Insert index %s failed. Attribution %s on Table %s is not unique!\n", indexName.c_str(), attrName.c_str(),tableName.c_str());
    }
    else
    {
        indexPage.writeIndex(tableName, attrName, indexName);
        //此处待补充，需要在catalogPage里面改当前属性上的索引总数，先push上去再说。。
        //此处应该有个api接口，用来真正建索引
        printf("Insert index %s(Attribution %s on Table %s) succeeded!\n", indexName.c_str(), attrName.c_str(),tableName.c_str());
    }
}

void CatalogManager::deleteIndex(string)
{
    //待补充，大概操作是，找这个索引，如果没有，gg
    //如果有，看在哪个表哪个属性，调用api接口真正删除这个索引
    //然后再indexCatalogPage里面删掉
    //再在catalogPage里面的对应属性上修改索引总数
}

int CatalogManager::indexNum(string tableName, string attrName)
{
    //待补充，先push上去再说。。。
    return 0;
}