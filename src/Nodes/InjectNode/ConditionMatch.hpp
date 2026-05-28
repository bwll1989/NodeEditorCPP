#pragma once

#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <cmath>

namespace Nodes
{
namespace ConditionMatch
{
    inline bool isStrictlyNumeric(const QVariant &value)
    {
        switch (value.typeId()) {
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
        case QMetaType::Float:
            return true;
        case QMetaType::QString: {
            bool ok = false;
            value.toString().toDouble(&ok);
            return ok;
        }
        default:
            return false;
        }
    }

    inline QVariant parseLiteral(const QString &text)
    {
        if (text.isEmpty()) {
            return {};
        }

        bool ok = false;
        const int intValue = text.toInt(&ok);
        if (ok) {
            return intValue;
        }

        const double doubleValue = text.toDouble(&ok);
        if (ok) {
            return doubleValue;
        }

        if (text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
            return true;
        }
        if (text.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
            return false;
        }

        return text;
    }

    inline QString normalizedText(const QVariant &value)
    {
        return value.toString().trimmed();
    }

    inline bool variantsEqual(const QVariant &lhs, const QVariant &rhs)
    {
        if (isStrictlyNumeric(lhs) && isStrictlyNumeric(rhs)) {
            return qFuzzyCompare(lhs.toDouble(), rhs.toDouble());
        }
        return normalizedText(lhs) == normalizedText(rhs);
    }

    inline bool compareVariants(const QVariant &lhs, const QVariant &rhs, const QString &op)
    {
        const bool numericCompare = isStrictlyNumeric(lhs) && isStrictlyNumeric(rhs);

        if (op == QStringLiteral("==")) {
            return variantsEqual(lhs, rhs);
        }
        if (op == QStringLiteral("!=")) {
            return !variantsEqual(lhs, rhs);
        }

        if (!numericCompare) {
            return false;
        }

        const double left = lhs.toDouble();
        const double right = rhs.toDouble();
        if (op == QStringLiteral(">")) {
            return left > right;
        }
        if (op == QStringLiteral("<")) {
            return left < right;
        }
        if (op == QStringLiteral(">=")) {
            return left >= right;
        }
        if (op == QStringLiteral("<=")) {
            return left <= right;
        }

        return false;
    }

    inline bool matches(const QVariant &input, const QString &conditionText)
    {
        const QString condition = conditionText.trimmed();
        if (condition.isEmpty()) {
            return false;
        }

        if (condition == QStringLiteral("*")) {
            return true;
        }

        static const QStringList operators = {
            QStringLiteral(">="),
            QStringLiteral("<="),
            QStringLiteral("!="),
            QStringLiteral("=="),
            QStringLiteral(">"),
            QStringLiteral("<"),
        };

        for (const QString &op : operators) {
            if (!condition.startsWith(op)) {
                continue;
            }

            const QString rhsText = condition.mid(op.size()).trimmed();
            return compareVariants(input, parseLiteral(rhsText), op);
        }

        return compareVariants(input, parseLiteral(condition), QStringLiteral("=="));
    }
} // namespace ConditionMatch
} // namespace Nodes
