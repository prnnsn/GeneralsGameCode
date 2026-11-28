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

#include "Common/INIValidationDiagnostic.h"
#include <vector>
#include <algorithm>

namespace INIValidation
{
	// A session belongs to exactly one validator invocation.
	// No global state or references to the engine's transient templates.
	class Session
	{
	public:
		explicit Session(Stage stage, const std::string& map = std::string())
			: m_stage(stage), m_map(stage == MAP_INI_VALIDATOR ? map : std::string())
		{
		}

		Stage stage() const { return m_stage; }
		const std::string& map() const { return m_map; }

		// Reject cross-stage records rather than mislabelling them.
		bool add(const Diagnostic& diagnostic)
		{
			if (diagnostic.stage != m_stage)
				return false;
			m_records.push_back(diagnostic);
			return true;
		}

		std::size_t count(Severity severity) const
		{
			std::size_t result = 0;
			for (std::size_t i = 0; i < m_records.size(); ++i)
				if (m_records[i].severity == severity)
					++result;
			return result;
		}

		std::size_t size() const { return m_records.size(); }

		// Sort the diagnostic representation only; never reorder engine data.
		// Retain duplicate records until a provenance-aware policy is defined.
		std::vector<std::string> formattedRecords() const
		{
			std::vector<std::string> records;
			records.reserve(m_records.size());
			for (std::size_t i = 0; i < m_records.size(); ++i)
				records.push_back(formatDiagnostic(m_records[i]));
			std::sort(records.begin(), records.end());
			return records;
		}

	private:
		Stage m_stage;
		std::string m_map;
		std::vector<Diagnostic> m_records;
	};
}
