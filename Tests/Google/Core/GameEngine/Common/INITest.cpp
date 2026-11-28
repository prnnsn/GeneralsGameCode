/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <cstdio>
#include <limits>
#include <gtest/gtest.h>

#include "Common/INI.h"
#include "Common/INIValidationDiagnostic.h"
#include "Common/INIValidationSession.h"

TEST(INI, OversizedIntegersMatchLegacyParser)
{
	const struct {
		const char* token;
		Int signedExpected;
		UnsignedInt unsignedExpected;
	} cases[] = {
		{"9999999999999999999",                               -1, 2313682943u},
		{"+9999999999999999999",                              -1, 2313682943u},
		{"-9999999999999999999",                               0, 1981284353u},
		{"9999999999999999999suffix",                         -1, 2313682943u},
		{"9223372036854775807",                               -1, 4294967295u},
		{"-9223372036854775808",                               0,          0u},
		{"9223372036854775808",                               -1,          0u},
		{"-9223372036854775809",                               0, 4294967295u},
		{"18446744073709551616",                              -1, 4294967295u},
		{"-18446744073709551616",                              0, 4294967295u},
		{"99999999999999999999999999999999999999999999999999", -1, 4294967295u}
	};
	// Windows legacy parsing saturates at 64 bits, then keeps the low 32 bits.
	for (const auto& test : cases)
	{
		SCOPED_TRACE(test.token);
		EXPECT_EQ(INI::scanInt(test.token), test.signedExpected);
		EXPECT_EQ(INI::scanUnsignedInt(test.token), test.unsignedExpected);
#ifdef _WIN32
		// Characterize the Windows CRT; out-of-range scanf conversions are not portable.
		Int legacySigned;
		UnsignedInt legacyUnsigned;
		ASSERT_EQ(std::sscanf(test.token, "%d", &legacySigned), 1);
		ASSERT_EQ(std::sscanf(test.token, "%u", &legacyUnsigned), 1);
		EXPECT_EQ(legacySigned, test.signedExpected);
		EXPECT_EQ(legacyUnsigned, test.unsignedExpected);
#endif
	}
}

TEST(INI, InvalidIntegersAreRejected)
{
	for (const char* token : {"abc", "+", "-", "++1", "+-99999999", "+-9999999999999999999"})
	{
		SCOPED_TRACE(token);
		EXPECT_THROW(INI::scanInt(token), decltype(INI_INVALID_DATA));
		EXPECT_THROW(INI::scanUnsignedInt(token), decltype(INI_INVALID_DATA));
		Int legacySigned;
		UnsignedInt legacyUnsigned;
		EXPECT_NE(std::sscanf(token, "%d", &legacySigned), 1);
		EXPECT_NE(std::sscanf(token, "%u", &legacyUnsigned), 1);
	}
}

TEST(INI, RealRangeErrorsMatchLegacyParser)
{
	const Real infinity = std::numeric_limits<Real>::infinity();
	const Real subnormal = std::numeric_limits<Real>::denorm_min();
	const struct {
		const char* token;
		Real expected;
	} cases[] = {
		{"1e100",         infinity},
		{"+1e100",        infinity},
		{"-1e100",       -infinity},
		{"3.5e38",        infinity},
		{"1e100suffix",   infinity},
		{"1e-100",            0.0f},
		{"+1e-100",           0.0f},
		{"-1e-100",          -0.0f},
		{"1e-45",        subnormal},
		{"-1e-45",      -subnormal},
		{"+1.25",            1.25f}
	};
	for (const auto& test : cases)
	{
		SCOPED_TRACE(test.token);
		const Real actual = INI::scanReal(test.token);
		EXPECT_EQ(actual, test.expected);
		EXPECT_EQ(std::signbit(actual), std::signbit(test.expected));
#ifdef _WIN32
		// As above, scanf is a Windows compatibility check, not a portable overflow oracle.
		Real legacy;
		ASSERT_EQ(std::sscanf(test.token, "%f", &legacy), 1);
		EXPECT_EQ(legacy, test.expected);
		EXPECT_EQ(std::signbit(legacy), std::signbit(test.expected));
#endif
	}
}

TEST(INI, InvalidRealsAreRejected)
{
	for (const char* token : {"abc", "+", "-", "++1.25", "+-1.25", "+-1e100", "+-1e-100"})
	{
		SCOPED_TRACE(token);
		EXPECT_THROW(INI::scanReal(token), decltype(INI_INVALID_DATA));
		Real legacy;
		EXPECT_NE(std::sscanf(token, "%f", &legacy), 1);
	}
}


TEST(INIValidation, DiagnosticIdentifiesValidatorAndSeverity)
{
	using namespace INIValidation;
	Diagnostic startup(INITIAL_INI_VALIDATOR, SEVERITY_DEFERRED, "REF_UPGRADE");
	startup.file = "Data/INI/Object/Test.ini";
	startup.line = 42;
	startup.entity = "TestTank";
	startup.module = "GrantUpgradeCreate";
	startup.moduleTag = "ModuleTag_Test";
	startup.field = "UpgradeToGrant";
	startup.value = "Upgrade_Missing";
	EXPECT_EQ(formatDiagnostic(startup),
		"[IIV] [DEFERRED] rule=\"REF_UPGRADE\" file=\"Data/INI/Object/Test.ini\" line=42 entity=\"TestTank\" module=\"GrantUpgradeCreate\" tag=\"ModuleTag_Test\" field=\"UpgradeToGrant\" value=\"Upgrade_Missing\"");

	Diagnostic map(MAP_INI_VALIDATOR, SEVERITY_ERROR, "REF_UPGRADE");
	EXPECT_EQ(formatDiagnostic(map), "[MIV] [ERROR] rule=\"REF_UPGRADE\"");
	EXPECT_STREQ(severityName(SEVERITY_VALID), "VALID");
	EXPECT_STREQ(severityName(SEVERITY_WARNING), "WARNING");
}

TEST(INIValidation, DiagnosticEscapesMalformedInputAndOmitsUnknownLine)
{
	using namespace INIValidation;
	Diagnostic diagnostic(MAP_INI_VALIDATOR, SEVERITY_WARNING, "UNKNOWN_FIELD");
	diagnostic.value = "a\n\r\t\"\\b";
	EXPECT_EQ(formatDiagnostic(diagnostic),
		"[MIV] [WARNING] rule=\"UNKNOWN_FIELD\" value=\"a\\x0A\\x0D\\x09\\\"\\\\b\"");
}


TEST(INIValidation, SessionKeepsStageAndMapIsolated)
{
	using namespace INIValidation;
	Session startup(INITIAL_INI_VALIDATOR, "unused");
	Diagnostic deferred(INITIAL_INI_VALIDATOR, SEVERITY_DEFERRED, "REF");
	Diagnostic error(MAP_INI_VALIDATOR, SEVERITY_ERROR, "REF");
	EXPECT_TRUE(startup.map().empty());
	EXPECT_TRUE(startup.add(deferred));
	EXPECT_FALSE(startup.add(error));
	EXPECT_EQ(startup.count(SEVERITY_DEFERRED), 1u);
	EXPECT_EQ(startup.count(SEVERITY_ERROR), 0u);

	Session mapA(MAP_INI_VALIDATOR, "Maps/A/A.map");
	EXPECT_TRUE(mapA.add(error));
	EXPECT_FALSE(mapA.add(deferred));
	EXPECT_EQ(mapA.count(SEVERITY_ERROR), 1u);
	EXPECT_EQ(mapA.map(), "Maps/A/A.map");
	Session mapB(MAP_INI_VALIDATOR, "Maps/B/B.map");
	EXPECT_EQ(mapB.size(), 0u);
	Session mapAAgain(MAP_INI_VALIDATOR, "Maps/A/A.map");
	EXPECT_EQ(mapAAgain.size(), 0u);
}

TEST(INIValidation, SessionFormatsRecordsInStableOrder)
{
	using namespace INIValidation;
	Session session(MAP_INI_VALIDATOR, "Maps/A/A.map");
	Diagnostic z(MAP_INI_VALIDATOR, SEVERITY_ERROR, "Z_RULE");
	Diagnostic a(MAP_INI_VALIDATOR, SEVERITY_ERROR, "A_RULE");
	ASSERT_TRUE(session.add(z));
	ASSERT_TRUE(session.add(a));
	ASSERT_TRUE(session.add(a));
	const std::vector<std::string> records = session.formattedRecords();
	ASSERT_EQ(records.size(), 3u);
	EXPECT_EQ(records[0], formatDiagnostic(a));
	EXPECT_EQ(records[1], formatDiagnostic(a));
	EXPECT_EQ(records[2], formatDiagnostic(z));
}
