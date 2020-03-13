#include <iostream>
#include "glfont.h"

Mesh generateTextGeometryBuffer(std::string text, float characterHeightOverWidth, float totalTextWidth) {
    float characterWidth = totalTextWidth / float(text.length());
    float characterHeight = characterHeightOverWidth * characterWidth;

    unsigned int vertexCount = 4 * text.length();
    unsigned int indexCount = 6 * text.length();

    Mesh mesh;

    mesh.vertices.resize(vertexCount);
    mesh.textureCoordinates.resize(vertexCount);
    mesh.indices.resize(indexCount);

    for(unsigned int i = 0; i < text.length(); i++)
    {
        float baseXCoordinate = float(i) * characterWidth;

        // Lower half triangle
        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0}; // bottom left
        mesh.vertices.at(4 * i + 1) = {baseXCoordinate + characterWidth, 0, 0}; // bottom right
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0}; //  top right

        // Upper half triangle
        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0}; // bottom left
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0}; // top right
        mesh.vertices.at(4 * i + 3) = {baseXCoordinate, characterHeight, 0}; // top left

        // Characters: 0 -> 127
        float uvX = float(text.at(i)) / 128.0f; // Gives a value 0->1 based on what character it is
        float unitVec = 1.0f / 128.0f;
        mesh.textureCoordinates.at(4 * i + 0) = {uvX, 0.0f}; // bottom left
        mesh.textureCoordinates.at(4 * i + 1) = {uvX + unitVec, 0.0f}; // bottom right
        mesh.textureCoordinates.at(4 * i + 2) = {uvX + unitVec, 1.0f}; // top right

        mesh.textureCoordinates.at(4 * i + 0) = {uvX, 0.0f}; // bottom left
        mesh.textureCoordinates.at(4 * i + 2) = {uvX + unitVec, 1.0f}; // top right
        mesh.textureCoordinates.at(4 * i + 3) = {uvX , 1.0f}; // top left

        mesh.indices.at(6 * i + 0) = 4 * i + 0;
        mesh.indices.at(6 * i + 1) = 4 * i + 1;
        mesh.indices.at(6 * i + 2) = 4 * i + 2;
        mesh.indices.at(6 * i + 3) = 4 * i + 0;
        mesh.indices.at(6 * i + 4) = 4 * i + 2;
        mesh.indices.at(6 * i + 5) = 4 * i + 3;
    }

    return mesh;
}