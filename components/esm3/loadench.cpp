#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

    void Enchantment::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();
        mEffects.mList.clear();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::fourCC("ENDT"):
                    esm.getHT(mData, 16);
                    hasData = true;
                    break;
                case ESM::fourCC("ENAM"):
                    mEffects.add(esm);
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
        if (!hasData && !isDeleted)
            esm.fail("Missing ENDT subrecord");
    }

    void Enchantment::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("ENDT", mData, 16);
        mEffects.save(esm);
    }

    void Enchantment::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mCharge = 0;
        mData.mFlags = 0;

        mEffects.mList.clear();
    }
}
