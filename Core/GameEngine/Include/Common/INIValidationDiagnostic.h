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

#pragma once

#include <string>
#include <sstream>
#include <locale>

namespace INIValidation
{
	enum Stage
	{
		INITIAL_INI_VALIDATOR,
		MAP_INI_VALIDATOR
	};

	enum Severity
	{
		SEVERITY_VALID,
		SEVERITY_DEFERRED,
		SEVERITY_WARNING,
		SEVERITY_ERROR
	};

	inline const char* stageName(Stage stage)
	{
		switch (stage)
		{
		case INITIAL_INI_VALIDATOR: return "IIV";
		case MAP_INI_VALIDATOR: return "MIV";
		}
		return "UNKNOWN";
	}

	inline const char* severityName(Severity severity)
	{
		switch (severity)
		{
		case SEVERITY_VALID: return "VALID";
		case SEVERITY_DEFERRED: return "DEFERRED";
		case SEVERITY_WARNING: return "WARNING";
		case SEVERITY_ERROR: return "ERROR";
		}
		return "UNKNOWN";
	}

	// Own the source strings: INI token buffers and map overrides are transient.
	// A line of zero means that no source line is available.
	struct Diagnostic
	{
		Diagnostic(Stage stage, Severity severity, const std::string& rule)
			: stage(stage), severity(severity), rule(rule), line(0)
		{
		}

		Stage stage;
		Severity severity;
		std::string rule;
		std::string file;
		unsigned int line;
		std::string entity;
		std::string module;
		std::string moduleTag;
		std::string field;
		std::string value;
		std::string message;
	};

	// Keep each diagnostic on one physical log line, even for malformed input.
	inline void appendQuoted(std::ostringstream& out, const std::string& value)
	{
		static const char hex[] = "0123456789ABCDEF";
		out << '"';
		for (std::string::size_type i = 0; i < value.size(); ++i)
		{
			const unsigned char c = static_cast<unsigned char>(value[i]);
			if (c == '\\' || c == '"')
				out << '\\' << static_cast<char>(c);
			else if (c < 32 || c == 127)
				out << "\\x" << hex[c >> 4] << hex[c & 15];
			else
				out << static_cast<char>(c);
		}
		out << '"';
	}

	inline void appendField(std::ostringstream& out, const char* key, const std::string& value)
	{
		if (!value.empty())
		{
			out << ' ' << key << '=';
			appendQuoted(out, value);
		}
	}

	// Formatting is side-effect free; no engine registries or game state are used.
	inline std::string formatDiagnostic(const Diagnostic& diagnostic)
	{
		std::ostringstream out;
		out.imbue(std::locale::classic());
		out << '[' << stageName(diagnostic.stage) << "] [" << severityName(diagnostic.severity) << ']';
		appendField(out, "rule", diagnostic.rule);
		appendField(out, "file", diagnostic.file);
		if (diagnostic.line != 0)
			out << " line=" << diagnostic.line;
		appendField(out, "entity", diagnostic.entity);
		appendField(out, "module", diagnostic.module);
		appendField(out, "tag", diagnostic.moduleTag);
		appendField(out, "field", diagnostic.field);
		appendField(out, "value", diagnostic.value);
		appendField(out, "message", diagnostic.message);
		return out.str();
	}
}
