#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

void ESM::DebugProfile::load (ESMReader& esm, bool &isDeleted)
{
    isDeleted = false;
    mRecordFlags = esm.getRecordFlags();

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        switch (esm.retSubName().toInt())
        {
            case ESM::SREC_NAME:
                mId = esm.getHString();
                break;
            case ESM::fourCC("DESC"):
                mDescription = esm.getHString();
                break;
            case ESM::fourCC("SCRP"):
                mScriptText = esm.getHString();
                break;
            case ESM::fourCC("FLAG"):
                esm.getHT(mFlags);
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
}

void ESM::DebugProfile::save (ESMWriter& esm, bool isDeleted) const
{
    esm.writeHNCString ("NAME", mId);

    if (isDeleted)
    {
        esm.writeHNString("DELE", "", 3);
        return;
    }

    esm.writeHNCString ("DESC", mDescription);
    esm.writeHNCString ("SCRP", mScriptText);
    esm.writeHNT ("FLAG", mFlags);
}

void ESM::DebugProfile::blank()
{
    mDescription.clear();
    mScriptText.clear();
    mFlags = 0;
}
