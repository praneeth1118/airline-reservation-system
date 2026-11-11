#ifndef EXCEL_EXPORTER_H
#define EXCEL_EXPORTER_H

#include <string>
#include <map>
#include "Ticket.h"
#include "Flight.h"

/**
 * @brief Handles automatic export of bookings and passenger lists
 * to Microsoft Excel (.xlsx and .xml) format with multiple worksheets.
 */
class ExcelExporter {
public:
    /**
     * @brief Exports all bookings to an OpenXML .xlsx workbook containing:
     *        - Sheet 1: Booking Details
     *        - Sheet 2: Passenger List
     */
    static bool exportToXlsx(const std::string& xlsxPath,
                             const std::map<std::string, Ticket>& tickets,
                             const std::map<std::string, Flight>& flights);

    /**
     * @brief Exports all bookings to an Excel XML Spreadsheet (XMLSS 2003) workbook
     *        supporting native multi-sheet opening in Microsoft Excel.
     */
    static bool exportToXmlSpreadsheet(const std::string& xmlPath,
                                       const std::map<std::string, Ticket>& tickets,
                                       const std::map<std::string, Flight>& flights);
};

#endif // EXCEL_EXPORTER_H

