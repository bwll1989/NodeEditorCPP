//
// Created by pablo on 2/29/24.
//

#ifndef VARIANTDATA_H
#define VARIANTDATA_H

#include <QtNodes/NodeData>
#include <QVariant>
using QtNodes::NodeData;
using QtNodes::NodeDataType;
// class VariantData:public NodeData{
// public:
//
//     VariantData(QVariant const &text=""):NodeValues(text){};
//     NodeDataType type() const override { return NodeDataType{"Variant", ""}; }
//     QVariant NodeValues;
// };

class VariantData final : public QtNodes::NodeData {
public:
    VariantData() = default;
    QVariant NodeValues;
    explicit VariantData(const QVariant& variant) : NodeValues(variant) {
    }

    NodeDataType type() const override {
        QString typeName = NodeValues.typeName();

        if (typeName.startsWith('Q') || typeName.startsWith('q')) {
            typeName = typeName.mid(1);
        }
        if(typeName=="") {
            typeName = "variant";
        }
        // // the string should have at least 10 characters, if not, fill with spaces
        // typeName = typeName.rightJustified(10, ' ');
        return QtNodes::NodeDataType{
            "variant",
            typeName
        };
    }


    bool isType(QMetaType::Type const type) const {
        return NodeValues.metaType().id() == type;
    }

    QMetaType::Type metaType() const {
        return static_cast<QMetaType::Type>(NodeValues.metaType().id());
    }

    QVariant variant() const {
        return NodeValues;
    }

    bool isValid() const {
        return NodeValues.isValid();
    }

    QString toString() const {
        QString output;
        switch (metaType()) {
            case QMetaType::QSize: {
                const QSize size = NodeValues.toSize();
                output = QString("%1x%2").arg(size.width()).arg(size.height());
                break;
            }
            case QMetaType::Bool: {
                output = NodeValues.toBool() ? "true" : "false";
                break;
            }
            default : {
                output = NodeValues.toString();
                break;
            }
        }

        return output;
    }

};
#endif //VARIANTDATA_H