#ifndef INDEXEDFACESET_H
#define INDEXEDFACESET_H

#include <vector>
#include <string>

struct vector2d
{
    float x;
    float y;
};

struct vector3d
{
    float x;
    float y;
    float z;
};

struct colour
{
    float r;
    float g;
    float b;
};

class IndexedFaceSet
{
public:
    IndexedFaceSet();
    IndexedFaceSet(const std::string& FileName);

private:
    std::vector<vector3d> coord;
    std::vector<int> coordIndex;
    std::vector<vector3d> normals;

    std::vector<vector2d> texCoord;
    std::vector<int> texCoordIndex;

    float creaseAngle;
    std::string textureFile;

};

#endif // INDEXEDFACESET_H
