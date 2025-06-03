#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H

#include <MyGUI_DataManager.h>

#include <memory>

#include "myguicompat.h"

namespace osgMyGUI
{

class DataManager : public MyGUI::DataManager
{
public:
    void initialise() {}
    void shutdown() {}

    void setResourcePath(const std::string& path);

    /** Get data stream from specified resource name.
        @param _name Resource name (usually file name).
    */
    MyGUI::IDataStream* getData(const std::string& _name) OPENMW_MYGUI_CONST_GETTER_3_4_1;

    /** Free data stream.
        @param _data Data stream.
    */
    void freeData(MyGUI::IDataStream* _data);

    /** Is data with specified name exist.
        @param _name Resource name.
    */
    bool isDataExist(const std::string& _name) OPENMW_MYGUI_CONST_GETTER_3_4_1;

    /** Get all data names with names that matches pattern.
        @param _pattern Pattern to match (for example "*.layout").
    */
    const MyGUI::VectorString& getDataListNames(const std::string& _pattern) OPENMW_MYGUI_CONST_GETTER_3_4_1;

    /** Get full path to data.
        @param _name Resource name.
        @return Return full path to specified data.
    */
    std::string getDataPath(const std::string& _name) OPENMW_MYGUI_CONST_GETTER_3_4_1;

private:
    std::string mResourcePath;
};

}

#endif
