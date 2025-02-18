#include "loadsndg.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int SoundGenerator::sRecordId = REC_SNDG;

    void SoundGenerator::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

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
                case ESM::fourCC("DATA"):
                    esm.getHT(mType, 4);
                    hasData = true;
                    break;
                case ESM::fourCC("CNAM"):
                    mCreature = esm.getHString();
                    break;
                case ESM::fourCC("SNAM"):
                    mSound = esm.getHString();
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
            esm.fail("Missing DATA subrecord");
    }
    void SoundGenerator::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mType, 4);
        esm.writeHNOCString("CNAM", mCreature);
        esm.writeHNOCString("SNAM", mSound);
        
    }

    void SoundGenerator::blank()
    {
        mType = LeftFoot;
        mCreature.clear();
        mSound.clear();
    }
}
