#include "loadstat.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Static::sRecordId = REC_STAT;

    void Static::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();
        //bool isBlocked = (mRecordFlags & ESM::FLAG_Blocked) != 0;
        //bool isPersistent = (mRecordFlags & ESM::FLAG_Persistent) != 0;

        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
    }
    void Static::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            esm.writeHNCString("MODL", mModel);
        }
    }

    void Static::blank()
    {
        mModel.clear();
    }
}
