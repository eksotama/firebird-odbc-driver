/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *	2003-03-24	IscColumnsResultSet.cpp
 *				Contributed by Norbert Meyer
 *				o Add some breaks to case statements in adjustResults()
 *				  VARCHAR storage length is set in function ::setCharLen
 *				o In IscColumnsResultSet::getBLRLiteral use delete[] s
 *				  instead of delete.s
 *
 *	2002-11-24	IscColumnsResultSet.cpp
 *				Contributed by C. G. Alvarez
 *				Improve handling of NUMERIC and DECIMAL fields
 *
 */


// IscColumnsResultSet.cpp: implementation of the IscColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#ifdef _WIN32
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscColumnsResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"
#include "IscSqlType.h"
#include "JString.h"

#define TYPE_NAME	6
#define DEF_VAL		13
#define BUFF_LEN	256

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnsResultSet::IscColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscColumnsResultSet::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	char sql[4096] =
		"select cast (NULL as varchar(7)) as table_cat,\n"			// 1 - VARCHAR
				"\tcast (NULL as varchar(7)) as table_schem,\n"		// 2 - VARCHAR
				"\tcast (rfr.rdb$relation_name as varchar(31)) as table_name,\n"	// 3 - VARCHAR NOT NULL
				"\tcast (rfr.rdb$field_name as varchar(31)) as column_name,\n"		// 4 - VARCHAR NOT NULL
				"\tfld.rdb$field_type as data_type,\n"				// 5 - SMALLINT NOT NULL
				"\tcast (fld.rdb$field_name as varchar(31)) as type_name,\n"		// 6 - VARCHAR NOT NULL
				"\tcast (fld.rdb$collation_id as integer) as column_size,\n"		// 7 - INTEGER
				"\tcast (fld.rdb$character_set_id as integer) as buffer_length,\n"	// 8 - INTEGER
				"\tcast (fld.rdb$field_scale as smallint) as decimal_digits,\n"		// 9 - SMALLINT
				"\tfld.rdb$field_scale as num_prec_radix,\n"		// 10 - SMALLINT
				"\trfr.rdb$null_flag as nullable,\n"				// 11 - SMALLINT NOT NULL
				"\tcast (NULL as char(10)) as remarks,\n"			// 12 - VARCHAR
				"\tcast (rfr.rdb$field_name as varchar(512)) as column_def,\n"				// 13 - VARCHAR
				"\tfld.rdb$field_type as SQL_DATA_TYPE,\n"			// 14 - SMALLINT NOT NULL
				"\tfld.rdb$field_sub_type as SQL_DATETIME_SUB,\n"	// 15 - SMALLINT
				"\t10 as CHAR_OCTET_LENGTH,\n"						// 16 - INTEGER
				"\t10 as ordinal_position,\n"						// 17 - INTEGER NOT NULL
				"\tcast ('YES' as varchar(3)) as IS_NULLABLE,\n"	// 18 - VARCHAR
				"\tfld.rdb$character_length as char_len,\n"			// 19
				"\tfld.rdb$default_value as f_def_val,\n"			// 20
				"\tfld.rdb$dimensions as array_dim,\n"				// 21
				"\tfld.rdb$null_flag as null_flag,\n"				// 22
				"\trfr.rdb$field_position as column_position,\n"	// 23
				"\tfld.rdb$field_length as column_length,\n"		// 24
				"\tfld.rdb$field_precision as column_precision,\n"	// 25
				"\trfr.rdb$default_value as column_def\n"			// 26
		"from rdb$relation_fields rfr, rdb$fields fld\n"
		"where rfr.rdb$field_source = fld.rdb$field_name\n";

	char * ptFirst = sql + strlen(sql);

	if ( !metaData->allTablesAreSelectable() )
		metaData->existsAccess(ptFirst, " and ", "rfr", 0, "\n");

	if (tableNamePattern && *tableNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$relation_name", tableNamePattern);

	if (fieldNamePattern && *fieldNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$field_name", fieldNamePattern);

	addString(ptFirst, " order by rfr.rdb$relation_name, rfr.rdb$field_position\n");
	
#ifdef DEBUG
	OutputDebugString (sql);
#endif
	prepareStatement (sql);

// SELECT returns 26 columns,
// But all interests only 18 
// This line is forbidden for modifying!!!
	numberColumns = 18;
}

bool IscColumnsResultSet::next()
{
	if (!IscResultSet::next())
	{
		blob.clear();
		return false;
	}

	int &len = sqlType.lengthIn;
	
	len = sqlda->getShort (24);

	sqlType.collationId = sqlda->getInt (7);		// COLLATION_ID
	sqlType.characterId = sqlda->getInt (8);		// CHARACTER_SET_ID

	sqlda->updateInt (7, len);						// COLUMN_SIZE
	sqlda->updateInt (8, len);						// BUFFER_LENGTH
	sqlda->updateShort (10, 10);					// NUM_PREC_RADIX
	sqlda->updateInt (16, len);						// CHAR_OCTET_LENGTH
	sqlda->updateInt (17, sqlda->getShort (23)+1);	// ORDINAL_POSITION
	
	//translate to the SQL type information
	sqlType.blrType	  = sqlda->getShort (5);		// DATA_TYPE
	sqlType.subType	  = sqlda->getShort (15);		// SUB_TYPE
	sqlType.scale	  = sqlda->getShort (9);		// DECIMAL_DIGITS
	int array	  = sqlda->getShort (21);			// ARRAY_DIMENSION
	sqlType.precision = sqlda->getShort (25);		// COLUMN_PRECISION
	sqlType.dialect = statement->connection->getDatabaseDialect();

	sqlType.buildType();

	if ( array )
	{
		int len;
		char * relation_name = sqlda->getVarying ( 3, len);
		relation_name[len] = '\0';
		char * field_name = sqlda->getVarying ( 4, len);
		field_name[len] = '\0';

		arrAttr.loadAttributes ( statement->connection, relation_name, field_name, sqlType.subType );

		sqlda->updateVarying (6, arrAttr.getFbSqlType());
		sqlda->updateInt (7, arrAttr.arrOctetLength );
		sqlda->updateInt (8, arrAttr.getBufferLength());

		if ( arrAttr.arrOctetLength < MAX_VARCHAR_LENGTH )
			sqlda->updateShort (5, JDBC_VARCHAR);
		else
			sqlda->updateShort (5, JDBC_LONGVARCHAR);

		sqlda->updateInt (16, arrAttr.arrOctetLength );
	}
	else
	{
		sqlda->updateVarying (6, sqlType.typeName);
		setCharLen (7, 8, sqlType);
		sqlda->updateShort (5, sqlType.type);

		//Octet length
		switch (sqlType.type)
		{
		case JDBC_VARCHAR:
		case JDBC_CHAR:
			sqlda->updateInt (16, sqlda->getInt (8));
			break;
		default:
			sqlda->setNull (16);
		} 
	}

	adjustResults (sqlType);

	return true;
}

bool IscColumnsResultSet::getBLRLiteral (int indexIn, 
										 int indexTarget,
										 IscSqlType &sqlType)
{
	if ( sqlda->isNull (indexIn) )
	{
		sqlda->updateVarying (indexTarget, "NULL");
		return false;
	}

	XSQLVAR *var = sqlda->Var(indexIn);

	blob.bind(statement->connection,(char*)var->sqldata);
	blob.setType(var->sqlsubtype);

	char * stuff = new char [blob.length()];
	char * s = stuff;

	for (int offset = 0, length; 
			length = blob.getSegmentLength(offset); 
			offset += length)
		memcpy (stuff + offset, blob.getSegment(offset), length);

	if ((*stuff != blr_version4) && (*stuff != blr_version5))
	{
		sqlda->updateVarying (indexTarget, "unknown, not BLR");
		delete[] s;
		return false;
	}

	stuff++;
	
	if (*stuff == blr_null)
	{
		sqlda->updateVarying (indexTarget, "NULL");
		delete[] s;	
		return true;
	}

	if (*stuff != blr_literal)
	{
		sqlda->updateVarying (indexTarget, "unknown, not literal");
		delete[] s;
		return false;
	}

	stuff++;	
	long	intVal, temp;
	short	type, scale, mag;
	char	stringTemp [BUFF_LEN];

	CFbDll * GDS = statement->connection->GDS;
	JString stringVal;
	TimeStamp timestamp;	
	type = *stuff++;

	switch (type)
	{
	case (blr_short):
	case (blr_long):
		scale = (*stuff++) * -1;
		mag = 1;

		intVal = GDS->_vax_integer (stuff, (type == blr_short)? 2 : 4);

		if (!scale)
			stringVal.Format ("%d", intVal);
		else
		{
			for (temp = intVal; scale; scale--)
			{
				temp /= 10;	
				mag *= 10;
			}
			intVal %= mag;
			scale = *--stuff * -1;			
			stringVal.Format ("%d.%0*d", temp, scale, intVal);
		}

		break;

	case (blr_quad):
	case (blr_int64):
		scale = (*stuff++) * -1;
		intVal = GDS->_vax_integer (stuff, 4);
		temp = GDS->_vax_integer (&stuff[4], 4);
		stringVal.Format ("0x%x%x scale %d", intVal, temp, scale);
		break;

	case (blr_float):
	case (blr_double):
		stringVal.Format ("%g", stuff);
		break;

	case (blr_d_float):
		stringVal.Format ("d_float is not an ODBC concept");
		break;
		
	case (blr_timestamp):
		timestamp.date = (long) stuff;
		timestamp.nanos = (long) &stuff[4];
		timestamp.getTimeString (BUFF_LEN, stringTemp);
		stringVal.Format ("\'%s\'", stringTemp);
		break;

	case (blr_sql_date):
		DateTime date;
		date.date = (long) stuff;
		date.getString (BUFF_LEN, stringTemp);
		stringVal.Format ("\'%s\'", stringTemp);
		break;

	case (blr_sql_time):
		SqlTime time;
		time.timeValue = (long) stuff;
		time.getString (BUFF_LEN, stringTemp);
		stringVal.Format ("\'%s\'", stringTemp);
		break;

	case (blr_text2):
	case (blr_varying2):
	case (blr_cstring2):
		stuff += 2;   // skip the type info
	case (blr_text):
	case (blr_varying):
	case (blr_cstring):
		switch (type)
		{
		case (blr_cstring):
		case (blr_cstring2):
			intVal = strlen (stuff);
			break;
		default:
			intVal = GDS->_vax_integer (stuff, 2);
		}
		if ((intVal + 4) >= BUFF_LEN)
		{
			stringVal = "TRUNCATED";
			break;
		}
		stringVal.setString (&stuff[2], intVal);
		checkQuotes (sqlType, stringVal); 
		break;
		
	case (blr_blob_id):
	case (blr_blob):
		stringVal = "blob type is not compatible";
		break;

	}
	sqlda->updateVarying (indexTarget, stringVal);
	delete[] s;
	return true;
}								

void IscColumnsResultSet::setCharLen (int charLenInd, 
								      int fldLenInd, 
									  IscSqlType &sqlType)
{
	int fldLen = sqlda->getInt (fldLenInd);
	int charLen = sqlda->getInt (charLenInd);

	if ( sqlda->isNull(charLenInd) )
		charLen = fldLen;

	if (sqlType.type != JDBC_VARCHAR &&
		sqlType.type != JDBC_CHAR)
	{
		charLen = sqlType.length;
		fldLen  = sqlType.bufferLength;
	}
	
	sqlda->updateInt (fldLenInd, fldLen);

	if (!charLen)
		sqlda->setNull (charLenInd);
	else
		sqlda->updateInt (charLenInd, charLen);
}

void IscColumnsResultSet::checkQuotes (IscSqlType &sqlType, JString stringVal)
{
	// Revolting ODBC wants the default value quoted unless its a 
	// number or a pseudo-literal
	
	JString	string = stringVal;
	string.upcase (string);

	switch (sqlType.type)
		{
		case JDBC_DATE:
		case JDBC_SQL_DATE:
		case JDBC_TIME:
		case JDBC_SQL_TIME:
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			if (string == "CURRENT DATE" ||
				string == "CURRENT TIME" ||
				string == "CURRENT TIMESTAMP" ||
				string == "CURRENT ROLE")
				{
				stringVal = string;
				return;
				}
		case JDBC_CHAR:
		case JDBC_VARCHAR:
			if (string == "USER")
			{
				stringVal = string;
				return;
			}
	}
	stringVal.Format ("\'%s\'", (const char *) stringVal);
	return;
}

void IscColumnsResultSet::adjustResults (IscSqlType &sqlType)
{
	// Data source�dependent data type name
	switch (sqlType.type)
	{
	case JDBC_LONGVARCHAR:
		sqlda->updateVarying (6, "BLOB SUB_TYPE TEXT");
		break;
	case JDBC_LONGVARBINARY:
		sqlda->updateVarying (6, "BLOB SUB_TYPE BLR");
		break;
	} 

	// decimal digits have no meaning for some columns
	// radix - doesn't mean much for some colums either
	switch (sqlType.type)
	{
	case JDBC_NUMERIC:
	case JDBC_DECIMAL:
		sqlda->updateShort (9, sqlda->getShort(9)*-1); 	// Scale > 0
		break;
	case JDBC_REAL:
	case JDBC_FLOAT:
	case JDBC_DOUBLE:
		sqlda->setNull (9);
		sqlda->updateShort (10, 2);
		break;
	case JDBC_CHAR:
	case JDBC_VARCHAR:
	case JDBC_LONGVARCHAR:
	case JDBC_LONGVARBINARY:
	case JDBC_DATE:
	case JDBC_SQL_DATE:
		sqlda->setNull (9);
		sqlda->setNull (10);
		break;
	case JDBC_TIME:
	case JDBC_SQL_TIME:
	case JDBC_TIMESTAMP:
	case JDBC_SQL_TIMESTAMP:
		sqlda->updateShort (9, -ISC_TIME_SECONDS_PRECISION_SCALE);
		sqlda->setNull (10);
	}	

	// nullable
	short nullable = !sqlda->getShort (11) || sqlda->isNull(11);
	sqlda->updateShort (11, nullable);

	// default values
	if (!getBLRLiteral (26, 13, sqlType))
		getBLRLiteral (20, 13, sqlType);

	switch (sqlType.type)
		{
		case JDBC_DATE:
		case JDBC_SQL_DATE:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 1);
			break;
		case JDBC_TIME:
		case JDBC_SQL_TIME:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 2);
			break;
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 3);
			break;
		default:
			sqlda->updateShort (14, sqlda->getShort(5));
			sqlda->setNull (15);
		}

	// Is Nullable - I'm seeing everything twice
	if ( !nullable )
		sqlda->updateVarying (18, "NO");
}

}; // end namespace IscDbcLibrary
