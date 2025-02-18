#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <unordered_set>
#include <map>
#include <fstream>
#include <cmath>
#include <memory>

#include <boost/program_options.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm/records.hpp>

#include "record.hpp"

#define ESMTOOL_VERSION 1.2

// Create a local alias for brevity
namespace bpo = boost::program_options;

struct ESMData
{
    std::string author;
    std::string description;
    unsigned int version;
    std::vector<ESM::Header::MasterData> masters;

    std::deque<std::unique_ptr<EsmTool::RecordBase>> mRecords;
    // Value: (Reference, Deleted flag)
    std::map<ESM::Cell *, std::deque<std::pair<ESM::CellRef, bool> > > mCellRefs;
    std::map<int, int> mRecordStats;

};

// Based on the legacy struct
struct Arguments
{
    bool raw_given;
    bool quiet_given;
    bool loadcells_given;
    bool plain_given;

    std::string mode;
    std::string encoding;
    std::string filename;
    std::string outname;

    std::vector<std::string> types;
    std::string name;

    ESMData data;
    ESM::ESMReader reader;
    ESM::ESMWriter writer;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc("Inspect and extract from Morrowind ES files (ESM, ESP, ESS)\nSyntax: esmtool [options] mode infile [outfile]\nAllowed modes:\n  dump\t Dumps all readable data from the input file.\n  clone\t Clones the input file to the output file.\n  comp\t Compares the given files.\n\nAllowed options");

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("raw,r", "Show an unformatted list of all records and subrecords.")
        // The intention is that this option would interact better
        // with other modes including clone, dump, and raw.
        ("type,t", bpo::value< std::vector<std::string> >(),
         "Show only records of this type (four character record code).  May "
         "be specified multiple times.  Only affects dump mode.")
        ("name,n", bpo::value<std::string>(),
         "Show only the record with this name.  Only affects dump mode.")
        ("plain,p", "Print contents of dialogs, books and scripts. "
         "(skipped by default)"
         "Only affects dump mode.")
        ("quiet,q", "Suppress all record information. Useful for speed tests.")
        ("loadcells,C", "Browse through contents of all cells.")

        ( "encoding,e", bpo::value<std::string>(&(info.encoding))->
          default_value("win1252"),
          "Character encoding used in ESMTool:\n"
          "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
          "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
          "\n\twin1252 - Western European (Latin) alphabet, used by default")
        ;

    std::string finalText = "\nIf no option is given, the default action is to parse all records in the archive\nand display diagnostic information.";

    // input-file is hidden and used as a positional argument
    bpo::options_description hidden("Hidden Options");

    hidden.add_options()
        ( "mode,m", bpo::value<std::string>(), "esmtool mode")
        ( "input-file,i", bpo::value< std::vector<std::string> >(), "input file")
        ;

    bpo::positional_options_description p;
    p.add("mode", 1).add("input-file", 2);

    // there might be a better way to do this
    bpo::options_description all;
    all.add(desc).add(hidden);
    bpo::variables_map variables;

    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
            .options(all).positional(p).run();

        bpo::store(valid_opts, variables);
    }
    catch(std::exception &e)
    {
        std::cout << "ERROR parsing arguments: " << e.what() << std::endl;
        return false;
    }

    bpo::notify(variables);

    if (variables.count ("help"))
    {
        std::cout << desc << finalText << std::endl;
        return false;
    }
    if (variables.count ("version"))
    {
        std::cout << "ESMTool version " << ESMTOOL_VERSION << std::endl;
        return false;
    }
    if (!variables.count("mode"))
    {
        std::cout << "No mode specified!\n\n"
                  << desc << finalText << std::endl;
        return false;
    }

    if (variables.count("type") > 0)
        info.types = variables["type"].as< std::vector<std::string> >();
    if (variables.count("name") > 0)
        info.name = variables["name"].as<std::string>();

    info.mode = variables["mode"].as<std::string>();
    if (!(info.mode == "dump" || info.mode == "clone" || info.mode == "comp"))
    {
        std::cout << "\nERROR: invalid mode \"" << info.mode << "\"\n\n"
                  << desc << finalText << std::endl;
        return false;
    }

    if ( !variables.count("input-file") )
    {
        std::cout << "\nERROR: missing ES file\n\n";
        std::cout << desc << finalText << std::endl;
        return false;
    }

    // handling gracefully the user adding multiple files
/*    if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
      {
      std::cout << "\nERROR: more than one ES file specified\n\n";
      std::cout << desc << finalText << std::endl;
      return false;
      }*/

    info.filename = variables["input-file"].as< std::vector<std::string> >()[0];
    if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
        info.outname = variables["input-file"].as< std::vector<std::string> >()[1];

    info.raw_given = variables.count ("raw") != 0;
    info.quiet_given = variables.count ("quiet") != 0;
    info.loadcells_given = variables.count ("loadcells") != 0;
    info.plain_given = variables.count("plain") != 0;

    // Font encoding settings
    info.encoding = variables["encoding"].as<std::string>();
    if(info.encoding != "win1250" && info.encoding != "win1251" && info.encoding != "win1252")
    {
        std::cout << info.encoding << " is not a valid encoding option.\n";
        info.encoding = "win1252";
    }
    std::cout << ToUTF8::encodingUsingMessage(info.encoding) << std::endl;

    return true;
}

void printRaw(ESM::ESMReader &esm);
void loadCell(ESM::Cell &cell, ESM::ESMReader &esm, Arguments& info);

int load(Arguments& info);
int clone(Arguments& info);
int comp(Arguments& info);

int main(int argc, char**argv)
{
    try
    {
        Arguments info;
        if(!parseOptions (argc, argv, info))
            return 1;

        if (info.mode == "dump")
            return load(info);
        else if (info.mode == "clone")
            return clone(info);
        else if (info.mode == "comp")
            return comp(info);
        else
        {
            std::cout << "Invalid or no mode specified, dying horribly. Have a nice day." << std::endl;
            return 1;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void loadCell(ESM::Cell &cell, ESM::ESMReader &esm, Arguments& info)
{
    bool quiet = (info.quiet_given || info.mode == "clone");
    bool save = (info.mode == "clone");

    // Skip back to the beginning of the reference list
    // FIXME: Changes to the references backend required to support multiple plugins have
    //  almost certainly broken this following line. I'll leave it as is for now, so that
    //  the compiler does not complain.
    cell.restore(esm, 0);

    // Loop through all the references
    ESM::CellRef ref;
    if(!quiet) std::cout << "  References:\n";

    bool deleted = false;
    ESM::MovedCellRef movedCellRef;
    bool moved = false;
    while(cell.getNextRef(esm, ref, deleted, movedCellRef, moved))
    {
        if (save) {
            info.data.mCellRefs[&cell].push_back(std::make_pair(ref, deleted));
        }

        if(quiet) continue;

        std::cout << "  - Refnum: " << ref.mRefNum.mIndex << '\n';
        std::cout << "    ID: " << ref.mRefID << '\n';
        std::cout << "    Position: (" << ref.mPos.pos[0] << ", " << ref.mPos.pos[1] << ", " << ref.mPos.pos[2] << ")\n";
        if (ref.mScale != 1.f)
            std::cout << "    Scale: " << ref.mScale << '\n';
        if (!ref.mOwner.empty())
            std::cout << "    Owner: " << ref.mOwner << '\n';
        if (!ref.mGlobalVariable.empty())
            std::cout << "    Global: " << ref.mGlobalVariable << '\n';
        if (!ref.mFaction.empty())
            std::cout << "    Faction: " << ref.mFaction << '\n';
        if (!ref.mFaction.empty() || ref.mFactionRank != -2)
            std::cout << "    Faction rank: " << ref.mFactionRank << '\n';
        std::cout << "    Enchantment charge: " << ref.mEnchantmentCharge << '\n';
        std::cout << "    Uses/health: " << ref.mChargeInt << '\n';
        std::cout << "    Gold value: " << ref.mGoldValue << '\n';
        std::cout << "    Blocked: " << static_cast<int>(ref.mReferenceBlocked) << '\n';
        std::cout << "    Deleted: " << deleted << '\n';
        if (!ref.mKey.empty())
            std::cout << "    Key: " << ref.mKey << '\n';
        std::cout << "    Lock level: " << ref.mLockLevel << '\n';
        if (!ref.mTrap.empty())
            std::cout << "    Trap: " << ref.mTrap << '\n';
        if (!ref.mSoul.empty())
            std::cout << "    Soul: " << ref.mSoul << '\n';
        if (ref.mTeleport)
        {
            std::cout << "    Destination position: (" << ref.mDoorDest.pos[0] << ", "
                      << ref.mDoorDest.pos[1] << ", " << ref.mDoorDest.pos[2] << ")\n";
            if (!ref.mDestCell.empty())
                std::cout << "    Destination cell: " << ref.mDestCell << '\n';
        }
        std::cout << "    Moved: " << std::boolalpha << moved << std::noboolalpha << '\n';
        if (moved)
        {
            std::cout << "    Moved refnum: " << movedCellRef.mRefNum.mIndex << '\n';
            std::cout << "    Moved content file: " << movedCellRef.mRefNum.mContentFile << '\n';
            std::cout << "    Target: " << movedCellRef.mTarget[0] << ", " << movedCellRef.mTarget[1] << '\n';
        }
    }
}

void printRaw(ESM::ESMReader &esm)
{
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        std::cout << "Record: " << n.toStringView() << '\n';
        esm.getRecHeader();
        while(esm.hasMoreSubs())
        {
            size_t offs = esm.getFileOffset();
            esm.getSubName();
            esm.skipHSub();
            n = esm.retSubName();
            std::ios::fmtflags f(std::cout.flags());
            std::cout << "    " << n.toStringView() << " - " << esm.getSubSize()
                 << " bytes @ 0x" << std::hex << offs << '\n';
            std::cout.flags(f);
        }
    }
}

int load(Arguments& info)
{
    ESM::ESMReader& esm = info.reader;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);

    std::string filename = info.filename;
    std::cout << "Loading file: " << filename << '\n';

    std::unordered_set<uint32_t> skipped;

    try {

        if(info.raw_given && info.mode == "dump")
        {
            std::cout << "RAW file listing:\n";

            esm.openRaw(filename);

            printRaw(esm);

            return 0;
        }

        bool quiet = (info.quiet_given || info.mode == "clone");
        bool loadCells = (info.loadcells_given || info.mode == "clone");
        bool save = (info.mode == "clone");

        esm.open(filename);

        info.data.author = esm.getAuthor();
        info.data.description = esm.getDesc();
        info.data.masters = esm.getGameFiles();

        if (!quiet)
        {
            std::cout << "Author: " << esm.getAuthor() << '\n'
                 << "Description: " << esm.getDesc() << '\n'
                 << "File format version: " << esm.getFVer() << '\n';
            std::vector<ESM::Header::MasterData> masterData = esm.getGameFiles();
            if (!masterData.empty())
            {
                std::cout << "Masters:" << '\n';
                for(const auto& master : masterData)
                    std::cout << "  " << master.name << ", " << master.size << " bytes\n";
            }
        }

        // Loop through all records
        while(esm.hasMoreRecs())
        {
            const ESM::NAME n = esm.getRecName();
            uint32_t flags;
            esm.getRecHeader(flags);

            auto record = EsmTool::RecordBase::create(n);
            if (record == nullptr)
            {
                if (!quiet && skipped.count(n.toInt()) == 0)
                {
                    std::cout << "Skipping " << n.toStringView() << " records.\n";
                    skipped.emplace(n.toInt());
                }

                esm.skipRecord();
                if (quiet) break;
                std::cout << "  Skipping\n";

                continue;
            }

            record->setFlags(static_cast<int>(flags));
            record->setPrintPlain(info.plain_given);
            record->load(esm);

            // Is the user interested in this record type?
            bool interested = true;
            if (!info.types.empty())
            {
                std::vector<std::string>::iterator match;
                match = std::find(info.types.begin(), info.types.end(), n.toStringView());
                if (match == info.types.end()) interested = false;
            }

            if (!info.name.empty() && !Misc::StringUtils::ciEqual(info.name, record->getId()))
                interested = false;

            if(!quiet && interested)
            {
                std::cout << "\nRecord: " << n.toStringView() << " '" << record->getId() << "'\n";
                record->print();
            }

            if (record->getType().toInt() == ESM::REC_CELL && loadCells && interested)
            {
                loadCell(record->cast<ESM::Cell>()->get(), esm, info);
            }

            if (save)
            {
                info.data.mRecords.push_back(std::move(record));
            }
            ++info.data.mRecordStats[n.toInt()];
        }

    } catch(std::exception &e) {
        std::cout << "\nERROR:\n\n  " << e.what() << std::endl;

        info.data.mRecords.clear();
        return 1;
    }

    return 0;
}

#include <iomanip>

int clone(Arguments& info)
{
    if (info.outname.empty())
    {
        std::cout << "You need to specify an output name" << std::endl;
        return 1;
    }

    if (load(info) != 0)
    {
        std::cout << "Failed to load, aborting." << std::endl;
        return 1;
    }

    size_t recordCount = info.data.mRecords.size();

    int digitCount = 1; // For a nicer output
    if (recordCount > 0)
        digitCount = (int)std::log10(recordCount) + 1;

    std::cout << "Loaded " << recordCount << " records:\n\n";

    int i = 0;
    for (std::pair<int, int> stat : info.data.mRecordStats)
    {
        ESM::NAME name;
        name = stat.first;
        int amount = stat.second;
        std::cout << std::setw(digitCount) << amount << " " << name.toStringView() << "  ";
        if (++i % 3 == 0)
            std::cout << '\n';
    }

    if (i % 3 != 0)
        std::cout << '\n';

    std::cout << "\nSaving records to: " << info.outname << "...\n";

    ESM::ESMWriter& esm = info.writer;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);
    esm.setAuthor(info.data.author);
    esm.setDescription(info.data.description);
    esm.setVersion(info.data.version);
    esm.setRecordCount (recordCount);

    for (const ESM::Header::MasterData &master : info.data.masters)
        esm.addMaster(master.name, master.size);

    std::fstream save(info.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    for (auto& record : info.data.mRecords)
    {
        if (i <= 0)
            break;

        const ESM::NAME typeName = record->getType();

        esm.startRecord(typeName, record->getFlags());

        record->save(esm);
        if (typeName.toInt() == ESM::REC_CELL) {
            ESM::Cell *ptr = &record->cast<ESM::Cell>()->get();
            if (!info.data.mCellRefs[ptr].empty()) 
            {
                for (std::pair<ESM::CellRef, bool> &ref : info.data.mCellRefs[ptr])
                    ref.first.save(esm, ref.second);
            }
        }

        esm.endRecord(typeName);

        saved++;
        int perc = recordCount == 0 ? 100 : (int)((saved / (float)recordCount)*100);
        if (perc % 10 == 0)
        {
            std::cerr << "\r" << perc << "%";
        }
    }

    std::cout << "\rDone!" << std::endl;

    esm.close();
    save.close();

    return 0;
}

int comp(Arguments& info)
{
    if (info.filename.empty() || info.outname.empty())
    {
        std::cout << "You need to specify two input files" << std::endl;
        return 1;
    }

    Arguments fileOne;
    Arguments fileTwo;

    fileOne.raw_given = false;
    fileTwo.raw_given = false;

    fileOne.mode = "clone";
    fileTwo.mode = "clone";

    fileOne.encoding = info.encoding;
    fileTwo.encoding = info.encoding;

    fileOne.filename = info.filename;
    fileTwo.filename = info.outname;

    if (load(fileOne) != 0)
    {
        std::cout << "Failed to load " << info.filename << ", aborting comparison." << std::endl;
        return 1;
    }

    if (load(fileTwo) != 0)
    {
        std::cout << "Failed to load " << info.outname << ", aborting comparison." << std::endl;
        return 1;
    }

    if (fileOne.data.mRecords.size() != fileTwo.data.mRecords.size())
    {
        std::cout << "Not equal, different amount of records." << std::endl;
        return 1;
    }

    return 0;
}
