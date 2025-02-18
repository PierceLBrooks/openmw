#include "loadcont.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{

    void InventoryList::add(ESMReader &esm)
    {
        esm.getSubHeader();
        ContItem ci;
        esm.getT(ci.mCount);
        ci.mItem.assign(esm.getString(32));
        mList.push_back(ci);
    }

    void InventoryList::save(ESMWriter &esm) const
    {
        for (std::vector<ContItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        {
            esm.startSubRecord("NPCO");
            esm.writeT(it->mCount);
            esm.writeFixedSizeString(it->mItem, 32);
            esm.endRecord("NPCO");
        }
    }

    unsigned int Container::sRecordId = REC_CONT;

    void Container::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mInventory.mList.clear();

        bool hasName = false;
        bool hasWeight = false;
        bool hasFlags = false;
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
                case ESM::fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case ESM::fourCC("CNDT"):
                    esm.getHT(mWeight, 4);
                    hasWeight = true;
                    break;
                case ESM::fourCC("FLAG"):
                    esm.getHT(mFlags, 4);
                    if (mFlags & 0xf4)
                        esm.fail("Unknown flags");
                    if (!(mFlags & 0x8))
                        esm.fail("Flag 8 not set");
                    hasFlags = true;
                    break;
                case ESM::fourCC("SCRI"):
                    mScript = esm.getHString();
                    break;
                case ESM::fourCC("NPCO"):
                    mInventory.add(esm);
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
        if (!hasWeight && !isDeleted)
            esm.fail("Missing CNDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }

    void Container::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CNDT", mWeight, 4);
        esm.writeHNT("FLAG", mFlags, 4);

        esm.writeHNOCString("SCRI", mScript);

        mInventory.save(esm);
    }

    void Container::blank()
    {
        mName.clear();
        mModel.clear();
        mScript.clear();
        mWeight = 0;
        mFlags = 0x8; // set default flag value
        mInventory.mList.clear();
    }
}
