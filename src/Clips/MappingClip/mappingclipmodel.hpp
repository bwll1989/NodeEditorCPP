#ifndef MAPPINGCLIPMODEL_H
#define MAPPINGCLIPMODEL_H


#include "TimeLineWidget/AbstractClipModel.hpp"

class MappingClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    enum MappingType {
        Linear,
        Exponential,
        Logarithmic,
        Step
    };

    MappingClipModel(int start, int end) 
        : AbstractClipModel(start, end, "Mapping") {
            EMBEDWIDGET = false;
        }
    ~MappingClipModel() override = default;
    // 映射特有属性
    MappingType getMappingType() const { return m_mappingType; }
    void setMappingType(MappingType type) { m_mappingType = type; }

    double getInputMin() const { return m_inputMin; }
    void setInputMin(double min) { m_inputMin = min; }

    double getInputMax() const { return m_inputMax; }
    void setInputMax(double max) { m_inputMax = max; }

    double getOutputMin() const { return m_outputMin; }
    void setOutputMin(double min) { m_outputMin = min; }

    double getOutputMax() const { return m_outputMax; }
    void setOutputMax(double max) { m_outputMax = max; }

    // 计算映射值
    double map(double input) const {
        double normalizedInput = (input - m_inputMin) / (m_inputMax - m_inputMin);
        double result;

        switch (m_mappingType) {
            case Linear:
                result = normalizedInput;
                break;
            case Exponential:
                result = normalizedInput * normalizedInput;
                break;
            case Logarithmic:
                result = std::log(1 + normalizedInput) / std::log(2);
                break;
            case Step:
                result = normalizedInput < 0.5 ? 0.0 : 1.0;
                break;
        }

        return m_outputMin + result * (m_outputMax - m_outputMin);
    }

    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["mappingType"] = static_cast<int>(m_mappingType);
        json["inputMin"] = m_inputMin;
        json["inputMax"] = m_inputMax;
        json["outputMin"] = m_outputMin;
        json["outputMax"] = m_outputMax;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_mappingType = static_cast<MappingType>(json["mappingType"].toInt());
        m_inputMin = json["inputMin"].toDouble();
        m_inputMax = json["inputMax"].toDouble();
        m_outputMin = json["outputMin"].toDouble();
        m_outputMax = json["outputMax"].toDouble();
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            return m_type;
        }
        return QVariant();
    }

    QVariant currentData(int currentFrame) const override {
        return QVariant();
    }

private:
    MappingType m_mappingType = Linear;  // 映射类型
    double m_inputMin = 0.0;   // 输入范围最小值
    double m_inputMax = 1.0;   // 输入范围最大值
    double m_outputMin = 0.0;  // 输出范围最小值
    double m_outputMax = 1.0;  // 输出范围最大值
};

#endif // MAPPINGCLIPMODEL_H 