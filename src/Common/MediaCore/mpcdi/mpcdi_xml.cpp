#include "mpcdi_parse.hpp"

// ============================================================================
// mpcdi.xml 解析 —— 对齐 bake.js parseMpcdiXml / indexFilesets / assetPath。
// XML 引擎 = tinyxml2（MPCDI 官方参考实现同款）。
// 要点：MPCDI 文件常带命名空间前缀（如 <mpc:display>），标签匹配须剥离前缀；
//       属性大小写存在工具差异（id/ID、xResolution/XResolution），逐个做兼容。
// ============================================================================

#include "tinyxml2.h"

namespace ms {

namespace {

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;

/** 剥离命名空间前缀："mpc:display" → "display"（对齐 bake.js localName）。 */
std::string local_name(const char* tag) {
    if (!tag) return {};
    const std::string t(tag);
    const size_t c = t.find(':');
    return c == std::string::npos ? t : t.substr(c + 1);
}

/** 元素直接子元素里按（去前缀）标签名过滤（对齐 bake.js childrenBy）。 */
std::vector<XMLElement*> children_by(XMLElement* el, const char* name) {
    std::vector<XMLElement*> out;
    if (!el) return out;
    for (XMLElement* c = el->FirstChildElement(); c; c = c->NextSiblingElement()) {
        if (local_name(c->Name()) == name) out.push_back(c);
    }
    return out;
}

XMLElement* first_child(XMLElement* el, const char* name) {
    const auto v = children_by(el, name);
    return v.empty() ? nullptr : v.front();
}

/** 元素第一个子元素的文本（int/float 解析用，容错 NaN/缺省）。 */
std::string child_text(XMLElement* el, const char* name) {
    XMLElement* c = first_child(el, name);
    if (!c || !c->GetText()) return {};
    const std::string t = c->GetText();
    // 首尾去空白
    size_t b = t.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    size_t e = t.find_last_not_of(" \t\r\n");
    return t.substr(b, e - b + 1);
}

/**
 * 资源路径提取（对齐 bake.js assetPath 优先级）：
 * 嵌套 <path> 文本 > path 属性 > 空元素直接文本。
 */
std::string asset_path(XMLElement* el) {
    if (!el) return {};
    if (XMLElement* p = first_child(el, "path")) {
        const std::string t = child_text(el, "path");
        if (!t.empty()) return t;
    }
    if (const char* attr = el->Attribute("path")) {
        if (attr[0] != '\0') return attr;
    }
    if (el->FirstChildElement() == nullptr && el->GetText()) {
        std::string t = el->GetText();
        size_t b = t.find_first_not_of(" \t\r\n");
        if (b != std::string::npos) {
            size_t e = t.find_last_not_of(" \t\r\n");
            return t.substr(b, e - b + 1);
        }
    }
    return {};
}

/** float 属性读取（对齐 bake.js floatAttr：非法或缺省回退 fallback）。 */
float float_attr(XMLElement* el, const char* name, float fallback) {
    if (!el) return fallback;
    const char* v = el->Attribute(name);
    if (!v) return fallback;
    char* end = nullptr;
    const double n = std::strtod(v, &end);
    return (end && end != v) ? static_cast<float>(n) : fallback;
}

/** int 属性读取（对齐 bake.js intAttr）。 */
int int_attr(XMLElement* el, const char* name, int fallback) {
    if (!el) return fallback;
    const char* v = el->Attribute(name);
    if (!v) return fallback;
    char* end = nullptr;
    const long n = std::strtol(v, &end, 10);
    return (end && end != v) ? static_cast<int>(n) : fallback;
}

/** 双名兼容取 int 属性：先 name，再首字母大写变体（xResolution/XResolution）。 */
int int_attr2(XMLElement* el, const char* a, const char* b, int fallback) {
    const int v = int_attr(el, a, INT32_MIN);
    if (v != INT32_MIN) return v;
    const int v2 = int_attr(el, b, INT32_MIN);
    return v2 != INT32_MIN ? v2 : fallback;
}

float float_attr2(XMLElement* el, const char* a, const char* b, float fallback) {
    const char* v = el ? el->Attribute(a) : nullptr;
    if (v) return float_attr(el, a, fallback);
    return float_attr(el, b, fallback);
}

/** alphaGamma 提取（对齐 bake.js indexFilesets：嵌套 gammaEmbedded 文本 > 属性 gammaEmbedded/gamma）。 */
float parse_alpha_gamma(XMLElement* alpha) {
    if (!alpha) return 0.0f;
    // 1) 嵌套 <gammaEmbedded> 文本
    const std::string g = child_text(alpha, "gammaEmbedded");
    if (!g.empty()) {
        char* end = nullptr;
        const double v = std::strtod(g.c_str(), &end);
        if (end && end != g.c_str()) return static_cast<float>(v);
    }
    // 2) alphaMap 元素上的属性
    if (const char* a = alpha->Attribute("gammaEmbedded")) {
        char* end = nullptr;
        const double v = std::strtod(a, &end);
        if (end && end != a) return static_cast<float>(v);
    }
    if (const char* a = alpha->Attribute("gamma")) {
        char* end = nullptr;
        const double v = std::strtod(a, &end);
        if (end && end != a) return static_cast<float>(v);
    }
    return 0.0f;
}

/** ASCII 小写整串（mpcdi.xml 备用名匹配用）。 */
std::string ascii_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// 主解析
// ---------------------------------------------------------------------------
bool parse_mpcdi_xml(const MpcdiPackageData& pkg, MpcdiInfo& out, std::string& err) {
    out = MpcdiInfo{};

    // 找 mpcdi.xml（对齐 bake.js：固定名优先，其次任意以 mpcdi.xml 结尾的条目）
    const std::vector<std::uint8_t>* xml_bytes = pkg.find("mpcdi.xml", {});
    if (!xml_bytes) {
        for (const auto& [name, bytes] : pkg.files) {
            const std::string lname = ascii_lower(name);
            if (lname.size() >= 9 && lname.compare(lname.size() - 9, 9, "mpcdi.xml") == 0) {
                xml_bytes = &bytes;
                break;
            }
        }
    }
    if (!xml_bytes) {
        err = "mpcdi.xml missing in package";
        return false;
    }

    XMLDocument doc;
    const tinyxml2::XMLError pe = doc.Parse(
        reinterpret_cast<const char*>(xml_bytes->data()), xml_bytes->size());
    if (pe != tinyxml2::XML_SUCCESS) {
        err = std::string("invalid mpcdi.xml: ") + doc.GetErrorStr1();
        return false;
    }
    XMLElement* root = doc.RootElement();
    if (!root) {
        err = "mpcdi.xml has no root element";
        return false;
    }

    if (const char* p = root->Attribute("profile")) out.profile = p;
    if (const char* v = root->Attribute("version")) out.version = v;

    // ---- fileset 索引（region id → 资源路径；region 内嵌元素优先级更高）----
    struct FilesetEntry {
        std::string geometry_warp_file;
        std::string alpha_map;
        std::string beta_map;
        float alpha_gamma = 0.0f;
    };
    std::map<std::string, FilesetEntry> filesets;

    // 递归收集全部元素（tinyxml2 无 getElementsByTagName("*")；文件小，开销可忽略）
    std::vector<XMLElement*> stack{root};
    std::vector<XMLElement*> all;
    while (!stack.empty()) {
        XMLElement* cur = stack.back();
        stack.pop_back();
        all.push_back(cur);
        for (XMLElement* c = cur->FirstChildElement(); c; c = c->NextSiblingElement()) {
            stack.push_back(c);
        }
    }
    for (XMLElement* el : all) {
        if (local_name(el->Name()) != "fileset") continue;
        const char* rid = el->Attribute("region");
        if (!rid) continue;
        FilesetEntry fs;
        if (XMLElement* geom = first_child(el, "geometryWarpFile"))
            fs.geometry_warp_file = asset_path(geom);
        if (XMLElement* alpha = first_child(el, "alphaMap")) {
            fs.alpha_map = asset_path(alpha);
            fs.alpha_gamma = parse_alpha_gamma(alpha);
        }
        if (XMLElement* beta = first_child(el, "betaMap"))
            fs.beta_map = asset_path(beta);
        filesets[rid] = std::move(fs);
    }

    // ---- display > buffer > region 结构 ----
    for (XMLElement* el : all) {
        if (local_name(el->Name()) != "display") continue;
        const char* d_id = el->Attribute("id");
        if (!d_id) d_id = el->Attribute("ID");
        const std::string display_id = d_id ? d_id : "display";

        for (XMLElement* buf : children_by(el, "buffer")) {
            const char* b_id = buf->Attribute("id");
            if (!b_id) b_id = buf->Attribute("ID");
            const std::string buffer_id = b_id ? b_id : "buffer";

            MpcdiBufferInfo bi;
            bi.id = buffer_id;
            bi.x_resolution = int_attr2(buf, "xResolution", "XResolution", 0);
            bi.y_resolution = int_attr2(buf, "yResolution", "YResolution", 0);
            out.buffers.push_back(bi);

            for (XMLElement* reg : children_by(buf, "region")) {
                const char* r_id = reg->Attribute("id");
                if (!r_id) r_id = reg->Attribute("ID");
                const std::string region_id = r_id ? r_id : "region";

                MpcdiRegionInfo r;
                r.id = region_id;
                r.key = display_id + "/" + buffer_id + "/" + region_id;
                r.display_id = display_id;
                r.buffer_id = buffer_id;
                r.x_resolution = int_attr2(reg, "xResolution", "XResolution", 0);
                r.y_resolution = int_attr2(reg, "yResolution", "YResolution", 0);
                r.x = float_attr(reg, "x", 0.0f);
                r.y = float_attr(reg, "y", 0.0f);
                r.x_size = float_attr2(reg, "xSize", "XSize", 1.0f);
                r.y_size = float_attr2(reg, "ySize", "YSize", 1.0f);

                // region 内嵌元素优先，其次 fileset 索引（对齐 bake.js 合并顺序）
                const auto fs_it = filesets.find(region_id);
                const FilesetEntry& fs = fs_it != filesets.end() ? fs_it->second : FilesetEntry{};
                if (XMLElement* geom = first_child(reg, "geometryWarpFile")) {
                    r.geometry_warp_file = asset_path(geom);
                }
                if (r.geometry_warp_file.empty()) r.geometry_warp_file = fs.geometry_warp_file;
                if (XMLElement* alpha = first_child(reg, "alphaMap")) {
                    r.alpha_map = asset_path(alpha);
                }
                if (r.alpha_map.empty()) r.alpha_map = fs.alpha_map;
                if (XMLElement* beta = first_child(reg, "betaMap")) {
                    r.beta_map = asset_path(beta);
                }
                if (r.beta_map.empty()) r.beta_map = fs.beta_map;
                r.alpha_gamma = fs.alpha_gamma;   // gamma 只来自 fileset（对齐 v26）

                out.regions.push_back(std::move(r));
            }
        }
    }

    if (out.regions.empty()) {
        err = "no regions in mpcdi.xml";
        return false;
    }
    err.clear();
    return true;
}

}  // namespace ms
