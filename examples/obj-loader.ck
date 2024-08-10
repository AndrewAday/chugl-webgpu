ObjLoader.load(me.dir() + "suzanne.obj") @=> Geometry @ geo;

geo.generateTangents();

// print the vertex arrays

T.println("// vertex positions:");
T.printArray(geo.vertexAttributeData(0), 1);
T.println("// vertex normals:");
T.printArray(geo.vertexAttributeData(1), 1);
T.println("// vertex uvs:");
T.printArray(geo.vertexAttributeData(2), 1);
T.println("// vertex tangents:");
T.printArray(geo.vertexAttributeData(3), 1);

<<< geo.vertexAttributeData(0).size() / 3 >>>;
<<< geo.vertexAttributeData(1).size() / 3 >>>;
<<< geo.vertexAttributeData(2).size() / 2 >>>;
<<< geo.vertexAttributeData(3).size() / 4 >>>;


// class implementation ==================================
public class ObjLoader
{
    class Vertex 
    {
        vec3 pos;
        vec3 norm;
        vec2 tex;
    }

    // loads a .obj file at given path into a custom geometry
    fun static Geometry@ load(string path)
    {
        // buffers for storing data from obj file
        vec3 positions[0];
        vec3 normals[0];
        vec2 texcoords[0];
        // buffer to generate vertex data from obj face indices 
        Vertex vertices[0];
        // instantiate a file IO object
        FileIO fio;
        // a string tokenizer
        StringTokenizer tokenizer;
        // a line of text
        string line;
        // a word token
        string word;

        // open the file
        fio.open( path, FileIO.READ );

        // ensure it's ok
        if( !fio.good() )
        {
            cherr <= "can't open file: " <= path <= " for reading..."
                <= IO.newline();
            me.exit();
        }

        // loop until end
        while( fio.more() )
        {
            // read current line
            fio.readLine() => line;
            // for each line, tokenize
            tokenizer.set( line );
            // loop over tokens
            while( tokenizer.more() )
            {
                // get the next word
                tokenizer.next() => word;

                // position data
                if (word == "v") {  
                    // get next three tokens as floats
                    positions << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // normal data
                } else if (word == "vn") {
                    normals << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // texture coordinate data
                } else if (word == "vt") {
                    texcoords << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // face data
                } else if (word == "f") {
                    // reconstruct vertices from face indices
                    repeat (3) {
                        Vertex v;
                        // zero out vertex attributes
                        @(0,0,0) => v.pos;
                        @(0,0,0) => v.norm;
                        @(0,0) => v.tex;
                        // get the next token
                        tokenizer.next() => string face;
                        face.replace( "//", "/0/");  // in case no texcoord is specified
                        face.replace( "/", " ");     // replace slashes with spaces
                        // tokenize it
                        StringTokenizer faceTokenizer;
                        faceTokenizer.set( face );

                        // NOTE: obj files are 1-indexed, so we subtract 1
                        // get the position index
                        Std.atoi(faceTokenizer.next()) - 1 => int posIndex;
                        // get the texture coordinate index
                        Std.atoi(faceTokenizer.next()) - 1 => int texIndex;
                        // get the normal index
                        Std.atoi(faceTokenizer.next()) - 1 => int normIndex;

                        if (posIndex >= 0) {
                            positions[posIndex] => v.pos;
                        }
                        if (texIndex >= 0) {
                            texcoords[texIndex] => v.tex;
                        }
                        if (normIndex >= 0) {
                            normals[normIndex] => v.norm;
                        }

                        if (v.pos == @(0,0,0)) {
                            <<< posIndex >>>;
                            <<< positions[posIndex] >>>;
                        }

                        vertices << v;
                    }
                }
            }
        }

        // At this point vertices contains all the vertex data
        // now we need to deconstruct it back buffers for each attribute
        // vec3 geoPositions[vertices.size()];
        // vec3 geoNormals[vertices.size()];
        // vec2 geoTexcoords[vertices.size()];
        float geoPositions[0];
        float geoNormals[0];
        float geoTexcoords[0];
        for (auto v : vertices) {
            geoPositions << v.pos.x;
            geoPositions << v.pos.y;
            geoPositions << v.pos.z;

            geoNormals << v.norm.x;
            geoNormals << v.norm.y;
            geoNormals << v.norm.z;

            geoTexcoords << v.tex.x;
            geoTexcoords << v.tex.y;

        }

        // assign to custom geometry
        Geometry geo;

        geo.vertexAttribute(0, 3, geoPositions);
        geo.vertexAttribute(1, 3, geoNormals);
        geo.vertexAttribute(2, 2, geoTexcoords);
        
        // TODO shorthands
        // customGeo.positions(geoPositions);
        // customGeo.normals(geoNormals);
        // customGeo.uvs(geoTexcoords);

        // T.printArray(texcoords);
        // T.println(texcoords.size() + "");

        return geo;
    }
}
