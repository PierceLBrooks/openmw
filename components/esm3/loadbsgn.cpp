#include "loadbsgn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int BirthSign::sRecordId = REC_BSGN;

    void BirthSign::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mPowers.mList.clear();

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
                case ESM::fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case ESM::fourCC("TNAM"):
                    mTexture = esm.getHString();
                    break;
                case ESM::fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                case ESM::fourCC("NPCS"):
                    mPowers.add(esm);
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

    void BirthSign::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("TNAM", mTexture);
        esm.writeHNOCString("DESC", mDescription);

        mPowers.save(esm);
    }

    void BirthSign::blank()
    {
        mName.clear();
        mDescription.clear();
        mTexture.clear();
        mPowers.mList.clear();
    }

}
