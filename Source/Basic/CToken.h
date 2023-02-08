#pragma once

class CToken
{
public:
										CToken();	
  virtual						~CToken();
	TokenType_e				GetType(void){ return m_Type; };
	void							SetType(TokenType_e type){ m_Type = type; };

private:
	CToken											*m_pNext;
	CToken											*m_pPrev;
	enum TokenType_e						m_Type;
	pValue_s										(*m_pStatement)(pValue_s pValue);
	union{
		/* T_CASEELSE           */ Casevalue_s *pCaseValue;
		/* T_DATA               */ PC_s NextData;
		/* T_DATAINPUT          */ char *pDataInput;
		/* T_DEFFN              */ Symbol_s *pLocalSyms;
		/* T_DO                 */ PC_s ExitDo;
		/* T_ELSE               */ PC_s EndIfPc;
		/* T_ELSEIFIF           */ PC_s ElsePc;
		/* T_END                */ PC_s EndPc;
		/* T_EQ                 */ enum ValueType_e Type;
		/* T_EXITFOR            */ PC_s ExitFor;
		/* T_GOSUB              */ PC_s GosubPc;
		/* T_GOTO               */ PC_s GotoPc;
		/* T_HEXINT							*/ long int HexInteger;
		/* T_OCTINT							*/ long int OctInteger;
		/* T_IDENTIFIER         */ Identifier_s *pIdentifier;
		/* T_INTEGER            */ long int Integer;
		/* T_JUNK               */ char Junk;
		/* T_LABEL							*/ Label_s *pLabel;
		/* T_LOOP               */ PC_s DoPc;
		/* T_NEXT               */ Next_s *pNext;
		/* T_ON                 */ On_s On;
		/* T_REAL               */ double Real;
		/* T_REM                */ char *pRem;
		/* T_RESTORE            */ PC_s Restore;
		/* T_SELECT             */ Selectcase_s *pSelectCase;
		/* T_STRING             */ String_s *pString;
		/* T_UNTIL              */ PC_s Until;
		/* T_USING              */ PC_s Image;
		/* T_WEND               */ PC_s *pWhilePc;
		/* T_WHILE              */ PC_s *pAfterWend;
	}													m_Obj;
};

