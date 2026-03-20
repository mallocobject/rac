#ifndef ELOG_FORMATTER_H
#define ELOG_FORMATTER_H

#include "elog/context.h"
#include "elog/flags.h"
#include <format>
#include <iterator>
#include <string>
namespace elog
{
class Formatter
{
  private:
	Flags flags_;

  public:
	explicit Formatter(Flags flags = Flags::kStdFlags) : flags_(flags)
	{
	}

	std::string format(const Context& ctx) const
	{
		return formatWithFormatTo(ctx);
	}

  private:
	std::string formatWithFormatTo(const Context& ctx) const
	{
		std::string formatted_log;
		auto out = std::back_inserter(formatted_log);

		std::format_to(out, "{}",
					   ctx.timestamp.toFormattedString(
						   static_cast<bool>(flags_ & Flags::kDate),
						   static_cast<bool>(flags_ & Flags::kTime)));

		if (flags_ & Flags::kThreadId)
		{
			std::format_to(out, "[{}]", ctx.tid);
		}

		std::format_to(out, "{} ", logLevel2String(ctx.level));

		if (flags_ & Flags::kFullName)
		{
			std::format_to(out, "{}", ctx.data.full_name);
		}
		else if (flags_ & Flags::kShortName)
		{
			std::format_to(out, "{}", ctx.data.short_name);
		}

		if (flags_ & Flags::kLine)
		{
			std::format_to(out, ":{} ", ctx.data.line);
		}

		if (flags_ & Flags::kFuncName)
		{
			std::format_to(out, "{}()", ctx.data.func);
		}

		std::format_to(out, "-> {}\n", ctx.text);

		return formatted_log;
	}

	static constexpr std::string_view logLevel2String(int level)
	{
		switch (level)
		{
		case 0:
			return "TRACE";
		case 1:
			return "DEBUG";
		case 2:
			return "INFO";
		case 3:
			return "WARN";
		case 4:
			return "ERROR";
		case 5:
			return "FATAL";
		default:
			return "UNKNOWN";
		}
	}
};
} // namespace elog

#endif