/*
 *        Copyright (c) 2020-2023 HydraSystems.
 *
 *  This program is free software; you can redistribute it and/or   
 *  modify it under the terms of the GNU General Public License as  
 *  published by the Free Software Foundation; either version 2 of  
 *  the License, or (at your option) any later version.             
 *                                                                  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   
 *  GNU General Public License for more details.                    
 *                                                                  
 *
 *  Based on a design by Michael Haardt
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * SJ   19/08/2020  Original
 *
 */

#include "stdafx.h"
#include "StringList.h"

////////////////////////////////////////////////////////////////////////////////////

StringListNode::StringListNode() noexcept
{
	m_pPrev = nullptr;
	m_pNext = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode::~StringListNode()
{
}

////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////

StringList::StringList() noexcept
{
	m_pHead = nullptr;
	m_pTail = nullptr;
	m_pCurrent = nullptr;
	m_Count = 0;
}

////////////////////////////////////////////////////////////////////////////////////

StringList::~StringList()
{
	while(Remove(0) != nullptr);
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode* StringList::GetAt(UINT Pos)
{
StringListNode *pNode = m_pTail;
UINT i = 0;

  if(Pos >= m_Count) return nullptr;
  while((++i < Pos) && (pNode != nullptr)) pNode = pNode->GetNext();
  return pNode;
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode* StringList::Add()
{
StringListNode *pNew, *pOldHead = nullptr;

  pNew = new StringListNode;													
  if(m_Count > 0){
  	pOldHead = m_pHead;													// save the pCurrent next node - maybe null if it's the head
  	m_pHead->SetNext(pNew);                 // set the new node as the existing head forward reference
  	m_pTail->SetPrev(pNew);                 // set the new node as the tail back reference
  	m_pHead = pNew;                             // plugin the new head in
  	m_pHead->SetPrev(pOldHead);             // set new head back reference
  	m_pHead->SetNext(m_pTail);              // set new head forward reference
  }
  else{
  	m_pTail = pNew;
  	m_pHead = pNew;
  	m_pHead->SetNext(pNew);
  	m_pHead->SetPrev(pNew);
  }
  ++m_Count;
  m_pCurrent = pNew;
  return pNew;
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode* StringList::Insert(UINT AfterPos)
{
StringListNode *pNew, *pNext = nullptr, *pAfter = m_pTail;
UINT i = 0;

  if((pAfter = GetAt(AfterPos)) == nullptr) return nullptr;
  pNew = new StringListNode;														// create new node
  if(pAfter != nullptr){
  	pNext = pAfter->GetNext();											// save the pCurrent next node - maybe null
  	pAfter->SetNext(pNew); 											// set the new pData
  }
  else  m_pHead = pNew; 									// this must be the head node	
  if(pNext != nullptr) pNext->SetPrev(pNew);
  else m_pTail = pNew; 									              // this must be the tail node. note this can be both head and tail
  pNew->SetPrev(pAfter);                         // this maybe null if this is the first node
  pNew->SetNext(pNext);
  ++m_Count;
  m_pCurrent = pNew;
  return pNew;
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode* StringList::Remove(UINT DelPos)
{
StringListNode *pPrev, *pNext, *pDel;

  if((pDel = GetAt(DelPos)) == nullptr) return nullptr;
  if(m_Count > 1){
	  pPrev = pDel->GetPrev();                              // get the link nodes
	  pNext = pDel->GetNext();
	  if(pPrev != nullptr) pPrev->SetNext(pNext);           // connect the two ends
	  if(pNext != nullptr) pNext->SetPrev(pPrev);
	  if(m_pHead == pDel) m_pHead = pPrev;                  // if we delete the head the previous becomes the head
	  if(m_pTail == pDel) m_pTail = pNext;                  // if we delete the tail the next becomes the tail
    if(pNext != nullptr)  m_pCurrent = pNext;
    else m_pCurrent = pPrev;
  }
  else{
    m_pHead = nullptr;
    m_pTail = nullptr;
    m_pCurrent = nullptr;
  }
	delete pDel;
	--m_Count;
  return m_pCurrent;
}

////////////////////////////////////////////////////////////////////////////////////

void StringList::RemoveAll()
{
	while(Remove(0) != nullptr);
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode*	StringList::GetNext()
{
  if(m_pCurrent != nullptr) m_pCurrent = m_pCurrent->GetNext();                // get the next node and make it current
  return m_pCurrent;
}

////////////////////////////////////////////////////////////////////////////////////

StringListNode*	StringList::GetPrev()
{
  if(m_pCurrent != nullptr) m_pCurrent = m_pCurrent->GetPrev();                // get the previous node and make it current
  return m_pCurrent;
}

////////////////////////////////////////////////////////////////////////////////////

UINT StringList::GetIndex(StringListNode *pRef)                                // get the position of a node 
{
StringListNode *pNode;
UINT Index = 0;

	if(m_Count == 0) return -1;
  pNode = m_pTail;
  while(pNode != nullptr){
  	if(pNode == pRef) return Index;
    pNode = pNode->GetNext();
  	++Index;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////////



