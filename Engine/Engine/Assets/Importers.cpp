#include "Importers.h"
#include <vector>
using namespace std;

namespace SIMPEngine
{
    static vector<unique_ptr<IAssetImporter>> *s_List = nullptr;
    vector<unique_ptr<IAssetImporter>> &ImporterRegistry::List()
    {
        if (!s_List)
            s_List = new vector<unique_ptr<IAssetImporter>>();

        return *s_List;
    }

    void ImporterRegistry::Register(unique_ptr<IAssetImporter> importer)
    {
        List().push_back(std::move(importer));
    }

    IAssetImporter *ImporterRegistry::Find(AssetType type)
    {
        for (auto &it : List())
        {
            if (it->Supports(type))
                return it.get();
        }
        return nullptr;
    }
}