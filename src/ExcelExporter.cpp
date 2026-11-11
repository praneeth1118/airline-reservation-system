#include "../include/ExcelExporter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>

// ----------------- Internal ZIP Packing Helpers -----------------
namespace {

uint32_t calculateCrc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (unsigned char b : data) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

void write16(std::ofstream& out, uint16_t val) {
    out.put(static_cast<char>(val & 0xFF));
    out.put(static_cast<char>((val >> 8) & 0xFF));
}

void write32(std::ofstream& out, uint32_t val) {
    out.put(static_cast<char>(val & 0xFF));
    out.put(static_cast<char>((val >> 8) & 0xFF));
    out.put(static_cast<char>((val >> 16) & 0xFF));
    out.put(static_cast<char>((val >> 24) & 0xFF));
}

struct ZipEntry {
    std::string name;
    std::string content;
    uint32_t crc;
    uint32_t offset;
};

bool writeZip(const std::string& zipPath, const std::vector<std::pair<std::string, std::string>>& files) {
    std::ofstream out(zipPath, std::ios::binary);
    if (!out.is_open()) return false;

    std::vector<ZipEntry> entries;

    for (const auto& f : files) {
        ZipEntry e;
        e.name = f.first;
        e.content = f.second;
        e.crc = calculateCrc32(e.content);
        e.offset = static_cast<uint32_t>(out.tellp());

        // Local file header (PK\x03\x04)
        write32(out, 0x04034b50);
        write16(out, 20); // Min version needed
        write16(out, 0);  // Flags
        write16(out, 0);  // Compression method (0 = STORED)
        write16(out, 0);  // File mod time
        write16(out, 0);  // File mod date
        write32(out, e.crc);
        write32(out, static_cast<uint32_t>(e.content.size()));
        write32(out, static_cast<uint32_t>(e.content.size()));
        write16(out, static_cast<uint16_t>(e.name.size()));
        write16(out, 0);  // Extra field len
        out.write(e.name.data(), e.name.size());
        out.write(e.content.data(), e.content.size());

        entries.push_back(e);
    }

    uint32_t cdOffset = static_cast<uint32_t>(out.tellp());

    // Central directory headers (PK\x01\x02)
    for (const auto& e : entries) {
        write32(out, 0x02014b50);
        write16(out, 20); // Made by
        write16(out, 20); // Version needed
        write16(out, 0);  // Flags
        write16(out, 0);  // Compression method (0 = STORED)
        write16(out, 0);  // Mod time
        write16(out, 0);  // Mod date
        write32(out, e.crc);
        write32(out, static_cast<uint32_t>(e.content.size()));
        write32(out, static_cast<uint32_t>(e.content.size()));
        write16(out, static_cast<uint16_t>(e.name.size()));
        write16(out, 0);  // Extra len
        write16(out, 0);  // Comment len
        write16(out, 0);  // Disk number start
        write16(out, 0);  // Internal attrs
        write32(out, 0);  // External attrs
        write32(out, e.offset); // Relative offset of local header
        out.write(e.name.data(), e.name.size());
    }

    uint32_t cdSize = static_cast<uint32_t>(out.tellp()) - cdOffset;

    // End of central directory record (PK\x05\x06)
    write32(out, 0x06054b50);
    write16(out, 0); // Disk number
    write16(out, 0); // Disk with central directory
    write16(out, static_cast<uint16_t>(entries.size()));
    write16(out, static_cast<uint16_t>(entries.size()));
    write32(out, cdSize);
    write32(out, cdOffset);
    write16(out, 0); // Comment len

    out.close();
    return true;
}

std::string xmlEscape(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&apos;";
        else out += c;
    }
    return out;
}

std::string getColLetter(int col) {
    std::string res;
    while (col > 0) {
        int rem = (col - 1) % 26;
        res = static_cast<char>('A' + rem) + res;
        col = (col - 1) / 26;
    }
    return res;
}

std::string formatDouble(double val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << val;
    return ss.str();
}

} // anonymous namespace

// ----------------- OpenXML .xlsx Export -----------------
bool ExcelExporter::exportToXlsx(const std::string& xlsxPath,
                                 const std::map<std::string, Ticket>& tickets,
                                 const std::map<std::string, Flight>& flights) {
    // 1. [Content_Types].xml
    std::string contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
        "  <Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>\n"
        "  <Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n"
        "  <Override PartName=\"/xl/worksheets/sheet2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n"
        "  <Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>\n"
        "</Types>";

    // 2. _rels/.rels
    std::string rootRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>\n"
        "</Relationships>";

    // 3. xl/_rels/workbook.xml.rels
    std::string wbRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>\n"
        "  <Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet2.xml\"/>\n"
        "  <Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>\n"
        "</Relationships>";

    // 4. xl/workbook.xml
    std::string workbook =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n"
        "  <sheets>\n"
        "    <sheet name=\"Booking Details\" sheetId=\"1\" r:id=\"rId1\"/>\n"
        "    <sheet name=\"Passenger List\" sheetId=\"2\" r:id=\"rId2\"/>\n"
        "  </sheets>\n"
        "</workbook>";

    // 5. xl/styles.xml (Headers: Navy background with white text; Cells: Clean border with alignments)
    std::string styles =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
        "  <fonts count=\"3\">\n"
        "    <font><name val=\"Calibri\"/><sz val=\"11\"/></font>\n"
        "    <font><b/><name val=\"Calibri\"/><sz val=\"11\"/><color rgb=\"FFFFFFFF\"/></font>\n"
        "    <font><name val=\"Calibri\"/><sz val=\"11\"/><color rgb=\"FF9C0006\"/></font>\n"
        "  </fonts>\n"
        "  <fills count=\"4\">\n"
        "    <fill><patternFill patternType=\"none\"/></fill>\n"
        "    <fill><patternFill patternType=\"gray125\"/></fill>\n"
        "    <fill><patternFill patternType=\"solid\"><fgColor rgb=\"FF1F497D\"/></patternFill></fill>\n"
        "    <fill><patternFill patternType=\"solid\"><fgColor rgb=\"FFFFC7CE\"/></patternFill></fill>\n"
        "  </fills>\n"
        "  <borders count=\"2\">\n"
        "    <border><left/><right/><top/><bottom/></border>\n"
        "    <border>\n"
        "      <left style=\"thin\"><color rgb=\"FFD9D9D9\"/></left>\n"
        "      <right style=\"thin\"><color rgb=\"FFD9D9D9\"/></right>\n"
        "      <top style=\"thin\"><color rgb=\"FFD9D9D9\"/></top>\n"
        "      <bottom style=\"thin\"><color rgb=\"FFD9D9D9\"/></bottom>\n"
        "    </border>\n"
        "  </borders>\n"
        "  <cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>\n"
        "  <cellXfs count=\"8\">\n"
        "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>\n"
        "    <xf numFmtId=\"0\" fontId=\"1\" fillId=\"2\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"center\" vertical=\"center\"/></xf>\n" // 1: Header (Center)
        "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"left\" vertical=\"center\"/></xf>\n"   // 2: Data Left
        "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"center\" vertical=\"center\"/></xf>\n" // 3: Data Center
        "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"right\" vertical=\"center\"/></xf>\n"  // 4: Data Right
        "    <xf numFmtId=\"0\" fontId=\"2\" fillId=\"3\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"left\" vertical=\"center\"/></xf>\n"   // 5: Cancelled Left
        "    <xf numFmtId=\"0\" fontId=\"2\" fillId=\"3\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"center\" vertical=\"center\"/></xf>\n" // 6: Cancelled Center
        "    <xf numFmtId=\"0\" fontId=\"2\" fillId=\"3\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\"><alignment horizontal=\"right\" vertical=\"center\"/></xf>\n"  // 7: Cancelled Right
        "  </cellXfs>\n"
        "</styleSheet>";

    // 6. xl/worksheets/sheet1.xml : "Booking Details"
    std::ostringstream s1;
    s1 << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
       << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
       << "  <cols>\n"
       << "    <col min=\"1\" max=\"1\" width=\"16\" customWidth=\"1\"/>\n"
       << "    <col min=\"2\" max=\"2\" width=\"14\" customWidth=\"1\"/>\n"
       << "    <col min=\"3\" max=\"3\" width=\"22\" customWidth=\"1\"/>\n"
       << "    <col min=\"4\" max=\"4\" width=\"26\" customWidth=\"1\"/>\n"
       << "    <col min=\"5\" max=\"5\" width=\"26\" customWidth=\"1\"/>\n"
       << "    <col min=\"6\" max=\"6\" width=\"24\" customWidth=\"1\"/>\n"
       << "    <col min=\"7\" max=\"7\" width=\"16\" customWidth=\"1\"/>\n"
       << "    <col min=\"8\" max=\"8\" width=\"15\" customWidth=\"1\"/>\n"
       << "    <col min=\"9\" max=\"9\" width=\"16\" customWidth=\"1\"/>\n"
       << "    <col min=\"10\" max=\"10\" width=\"18\" customWidth=\"1\"/>\n"
       << "    <col min=\"11\" max=\"11\" width=\"22\" customWidth=\"1\"/>\n"
       << "    <col min=\"12\" max=\"12\" width=\"16\" customWidth=\"1\"/>\n"
       << "  </cols>\n"
       << "  <sheetData>\n"
       << "    <row r=\"1\" ht=\"25\" customHeight=\"1\">\n";

    std::vector<std::string> s1Headers = {
        "Ticket ID", "Flight ID", "Airline", "Origin", "Destination",
        "Passenger Name", "Seat Class", "Seat Number", "Fare Paid ($)",
        "Refund Amount ($)", "Booking Date", "Status"
    };

    for (size_t c = 0; c < s1Headers.size(); ++c) {
        std::string ref = getColLetter(static_cast<int>(c + 1)) + "1";
        s1 << "      <c r=\"" << ref << "\" t=\"inlineStr\" s=\"1\"><is><t>"
           << xmlEscape(s1Headers[c]) << "</t></is></c>\n";
    }
    s1 << "    </row>\n";

    int rowIdx = 2;
    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        auto p = t.getPassenger();
        auto s = t.getSeatClass();
        std::string flNo = t.getFlightNumber();
        std::string origin = "", dest = "", airline = "";
        auto flIt = flights.find(flNo);
        if (flIt != flights.end()) {
            origin = flIt->second.getOrigin();
            dest = flIt->second.getDestination();
            airline = flIt->second.getAirlineName();
        }

        bool isCancelled = (t.getStatus() == "CANCELLED");
        std::string styleLeft   = isCancelled ? "5" : "2";
        std::string styleCenter = isCancelled ? "6" : "3";
        std::string styleRight  = isCancelled ? "7" : "4";

        s1 << "    <row r=\"" << rowIdx << "\" ht=\"20\" customHeight=\"1\">\n";

        auto writeStr = [&](int col, const std::string& val, const std::string& st) {
            std::string ref = getColLetter(col) + std::to_string(rowIdx);
            s1 << "      <c r=\"" << ref << "\" t=\"inlineStr\" s=\"" << st << "\"><is><t>"
               << xmlEscape(val) << "</t></is></c>\n";
        };

        auto writeNum = [&](int col, double val, const std::string& st) {
            std::string ref = getColLetter(col) + std::to_string(rowIdx);
            s1 << "      <c r=\"" << ref << "\" s=\"" << st << "\"><v>"
               << formatDouble(val) << "</v></c>\n";
        };

        writeStr(1, t.getTicketId(), styleCenter);
        writeStr(2, t.getFlightNumber(), styleCenter);
        writeStr(3, airline, styleLeft);
        writeStr(4, origin, styleLeft);
        writeStr(5, dest, styleLeft);
        writeStr(6, p.getFullName(), styleLeft);
        writeStr(7, s ? s->getClassName() : "Unknown", styleCenter);
        writeStr(8, t.getSeatNumber(), styleCenter);
        writeNum(9, t.getFarePaid(), styleRight);
        writeNum(10, t.getRefundAmount(), styleRight);
        writeStr(11, t.getBookingDate(), styleCenter);
        writeStr(12, t.getStatus(), styleCenter);

        s1 << "    </row>\n";
        rowIdx++;
    }
    s1 << "  </sheetData>\n"
       << "</worksheet>";

    // 7. xl/worksheets/sheet2.xml : "Passenger List"
    std::ostringstream s2;
    s2 << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
       << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
       << "  <cols>\n"
       << "    <col min=\"1\" max=\"1\" width=\"24\" customWidth=\"1\"/>\n"
       << "    <col min=\"2\" max=\"2\" width=\"24\" customWidth=\"1\"/>\n"
       << "    <col min=\"3\" max=\"3\" width=\"10\" customWidth=\"1\"/>\n"
       << "    <col min=\"4\" max=\"4\" width=\"12\" customWidth=\"1\"/>\n"
       << "    <col min=\"5\" max=\"5\" width=\"28\" customWidth=\"1\"/>\n"
       << "    <col min=\"6\" max=\"6\" width=\"20\" customWidth=\"1\"/>\n"
       << "    <col min=\"7\" max=\"7\" width=\"18\" customWidth=\"1\"/>\n"
       << "    <col min=\"8\" max=\"8\" width=\"14\" customWidth=\"1\"/>\n"
       << "    <col min=\"9\" max=\"9\" width=\"15\" customWidth=\"1\"/>\n"
       << "    <col min=\"10\" max=\"10\" width=\"16\" customWidth=\"1\"/>\n"
       << "    <col min=\"11\" max=\"11\" width=\"16\" customWidth=\"1\"/>\n"
       << "  </cols>\n"
       << "  <sheetData>\n"
       << "    <row r=\"1\" ht=\"25\" customHeight=\"1\">\n";

    std::vector<std::string> s2Headers = {
        "Passenger ID / Ticket ID", "Passenger Name", "Age", "Gender",
        "Email", "Phone Number", "Passport Number", "Flight ID",
        "Seat Number", "Seat Class", "Booking Status"
    };

    for (size_t c = 0; c < s2Headers.size(); ++c) {
        std::string ref = getColLetter(static_cast<int>(c + 1)) + "1";
        s2 << "      <c r=\"" << ref << "\" t=\"inlineStr\" s=\"1\"><is><t>"
           << xmlEscape(s2Headers[c]) << "</t></is></c>\n";
    }
    s2 << "    </row>\n";

    rowIdx = 2;
    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        auto p = t.getPassenger();
        auto s = t.getSeatClass();
        bool isCancelled = (t.getStatus() == "CANCELLED");
        std::string styleLeft   = isCancelled ? "5" : "2";
        std::string styleCenter = isCancelled ? "6" : "3";
        std::string styleRight  = isCancelled ? "7" : "4";

        s2 << "    <row r=\"" << rowIdx << "\" ht=\"20\" customHeight=\"1\">\n";

        auto writeStr = [&](int col, const std::string& val, const std::string& st) {
            std::string ref = getColLetter(col) + std::to_string(rowIdx);
            s2 << "      <c r=\"" << ref << "\" t=\"inlineStr\" s=\"" << st << "\"><is><t>"
               << xmlEscape(val) << "</t></is></c>\n";
        };

        auto writeInt = [&](int col, int val, const std::string& st) {
            std::string ref = getColLetter(col) + std::to_string(rowIdx);
            s2 << "      <c r=\"" << ref << "\" s=\"" << st << "\"><v>"
               << val << "</v></c>\n";
        };

        writeStr(1, t.getTicketId(), styleCenter);
        writeStr(2, p.getFullName(), styleLeft);
        writeInt(3, p.getAge(), styleCenter);
        writeStr(4, p.getGender(), styleCenter);
        writeStr(5, p.getEmail(), styleLeft);
        writeStr(6, p.getPhone(), styleCenter);
        writeStr(7, p.getPassportNumber(), styleCenter);
        writeStr(8, t.getFlightNumber(), styleCenter);
        writeStr(9, t.getSeatNumber(), styleCenter);
        writeStr(10, s ? s->getClassName() : "Unknown", styleCenter);
        writeStr(11, t.getStatus(), styleCenter);

        s2 << "    </row>\n";
        rowIdx++;
    }
    s2 << "  </sheetData>\n"
       << "</worksheet>";

    std::vector<std::pair<std::string, std::string>> files = {
        {"[Content_Types].xml", contentTypes},
        {"_rels/.rels", rootRels},
        {"xl/_rels/workbook.xml.rels", wbRels},
        {"xl/workbook.xml", workbook},
        {"xl/styles.xml", styles},
        {"xl/worksheets/sheet1.xml", s1.str()},
        {"xl/worksheets/sheet2.xml", s2.str()}
    };

    return writeZip(xlsxPath, files);
}

// ----------------- Excel XML Spreadsheet 2003 Export -----------------
bool ExcelExporter::exportToXmlSpreadsheet(const std::string& xmlPath,
                                           const std::map<std::string, Ticket>& tickets,
                                           const std::map<std::string, Flight>& flights) {
    std::ofstream out(xmlPath);
    if (!out.is_open()) return false;

    out << "<?xml version=\"1.0\"?>\n"
        << "<?mso-application progid=\"Excel.Sheet\"?>\n"
        << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"
        << " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n"
        << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n"
        << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"
        << " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n"
        << " <Styles>\n"
        << "  <Style ss:ID=\"Default\" ss:Name=\"Normal\">\n"
        << "   <Alignment ss:Vertical=\"Center\"/>\n"
        << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\"/>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"Header\">\n"
        << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n"
        << "   </Borders>\n"
        << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Color=\"#FFFFFF\" ss:Bold=\"1\"/>\n"
        << "   <Interior ss:Color=\"#1F497D\" ss:Pattern=\"Solid\"/>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"DataLeft\">\n"
        << "   <Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"DataCenter\">\n"
        << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"DataRight\">\n"
        << "   <Alignment ss:Horizontal=\"Right\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"CancelledLeft\">\n"
        << "   <Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Color=\"#9C0006\"/>\n"
        << "   <Interior ss:Color=\"#FFC7CE\" ss:Pattern=\"Solid\"/>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"CancelledCenter\">\n"
        << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Color=\"#9C0006\"/>\n"
        << "   <Interior ss:Color=\"#FFC7CE\" ss:Pattern=\"Solid\"/>\n"
        << "  </Style>\n"
        << "  <Style ss:ID=\"CancelledRight\">\n"
        << "   <Alignment ss:Horizontal=\"Right\" ss:Vertical=\"Center\"/>\n"
        << "   <Borders>\n"
        << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n"
        << "   </Borders>\n"
        << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Color=\"#9C0006\"/>\n"
        << "   <Interior ss:Color=\"#FFC7CE\" ss:Pattern=\"Solid\"/>\n"
        << "  </Style>\n"
        << " </Styles>\n";

    // Sheet 1: Booking Details
    out << " <Worksheet ss:Name=\"Booking Details\">\n"
        << "  <Table>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Column ss:Width=\"95\"/>\n"
        << "   <Column ss:Width=\"150\"/>\n"
        << "   <Column ss:Width=\"160\"/>\n"
        << "   <Column ss:Width=\"160\"/>\n"
        << "   <Column ss:Width=\"160\"/>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Column ss:Width=\"95\"/>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Column ss:Width=\"120\"/>\n"
        << "   <Column ss:Width=\"140\"/>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Row ss:Height=\"24\">\n";

    std::vector<std::string> s1Headers = {
        "Ticket ID", "Flight ID", "Airline", "Origin", "Destination",
        "Passenger Name", "Seat Class", "Seat Number", "Fare Paid ($)",
        "Refund Amount ($)", "Booking Date", "Status"
    };

    for (const auto& h : s1Headers) {
        out << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">" << xmlEscape(h) << "</Data></Cell>\n";
    }
    out << "   </Row>\n";

    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        auto p = t.getPassenger();
        auto s = t.getSeatClass();
        std::string flNo = t.getFlightNumber();
        std::string origin = "", dest = "", airline = "";
        auto flIt = flights.find(flNo);
        if (flIt != flights.end()) {
            origin = flIt->second.getOrigin();
            dest = flIt->second.getDestination();
            airline = flIt->second.getAirlineName();
        }

        bool isCancelled = (t.getStatus() == "CANCELLED");
        std::string styleLeft   = isCancelled ? "CancelledLeft" : "DataLeft";
        std::string styleCenter = isCancelled ? "CancelledCenter" : "DataCenter";
        std::string styleRight  = isCancelled ? "CancelledRight" : "DataRight";

        out << "   <Row ss:Height=\"19\">\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getTicketId()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getFlightNumber()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(airline) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(origin) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(dest) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getFullName()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(s ? s->getClassName() : "Unknown") << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getSeatNumber()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleRight << "\"><Data ss:Type=\"Number\">" << formatDouble(t.getFarePaid()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleRight << "\"><Data ss:Type=\"Number\">" << formatDouble(t.getRefundAmount()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getBookingDate()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getStatus()) << "</Data></Cell>\n"
            << "   </Row>\n";
    }
    out << "  </Table>\n"
        << " </Worksheet>\n";

    // Sheet 2: Passenger List
    out << " <Worksheet ss:Name=\"Passenger List\">\n"
        << "  <Table>\n"
        << "   <Column ss:Width=\"150\"/>\n"
        << "   <Column ss:Width=\"160\"/>\n"
        << "   <Column ss:Width=\"65\"/>\n"
        << "   <Column ss:Width=\"80\"/>\n"
        << "   <Column ss:Width=\"190\"/>\n"
        << "   <Column ss:Width=\"135\"/>\n"
        << "   <Column ss:Width=\"120\"/>\n"
        << "   <Column ss:Width=\"95\"/>\n"
        << "   <Column ss:Width=\"95\"/>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Column ss:Width=\"110\"/>\n"
        << "   <Row ss:Height=\"24\">\n";

    std::vector<std::string> s2Headers = {
        "Passenger ID / Ticket ID", "Passenger Name", "Age", "Gender",
        "Email", "Phone Number", "Passport Number", "Flight ID",
        "Seat Number", "Seat Class", "Booking Status"
    };

    for (const auto& h : s2Headers) {
        out << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">" << xmlEscape(h) << "</Data></Cell>\n";
    }
    out << "   </Row>\n";

    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        auto p = t.getPassenger();
        auto s = t.getSeatClass();
        bool isCancelled = (t.getStatus() == "CANCELLED");
        std::string styleLeft   = isCancelled ? "CancelledLeft" : "DataLeft";
        std::string styleCenter = isCancelled ? "CancelledCenter" : "DataCenter";

        out << "   <Row ss:Height=\"19\">\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getTicketId()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getFullName()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"Number\">" << p.getAge() << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getGender()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleLeft << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getEmail()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getPhone()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(p.getPassportNumber()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getFlightNumber()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getSeatNumber()) << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(s ? s->getClassName() : "Unknown") << "</Data></Cell>\n"
            << "    <Cell ss:StyleID=\"" << styleCenter << "\"><Data ss:Type=\"String\">" << xmlEscape(t.getStatus()) << "</Data></Cell>\n"
            << "   </Row>\n";
    }
    out << "  </Table>\n"
        << " </Worksheet>\n"
        << "</Workbook>\n";

    out.close();
    return true;
}

