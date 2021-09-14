from PBRTv3Loader import *
from pathlib import *
import os
import json as pyJson
from math import *


# assumes row major matrix, doesnt work for negative scales
# https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati/417813
# https://stackoverflow.com/questions/15022630/how-to-calculate-the-angle-from-rotation-matrix
def SeparateOutTransformMatrix( m ):
    position = [ m[0][3], m[1][3], m[2][3] ]
    scale = [ sqrt( m[0][0]**2 + m[1][0]**2 + m[2][0]**2), sqrt( m[0][1] ** 2 + m[1][1] ** 2 + m[2][1] ** 2), sqrt( m[0][2] ** 2 + m[1][2] ** 2 + m[2][2] ** 2) ]
    R = [ [m[0][0] / scale[0], m[0][1] / scale[1], m[0][2] / scale[2]], [m[1][0] / scale[0], m[1][1] / scale[1], m[1][2] / scale[2]], [m[2][0] / scale[0], m[2][1] / scale[1], m[2][2] / scale[2]] ]
    rotation = [ atan2( R[2][1], R[2][2] ), atan2( -R[2][0], sqrt(R[2][1]**2 + R[2][2]**2)), atan2( R[1][0], R[0][0] ) ]

    return position, rotation, scale


def RelativeAssetPath( relPathStr, sceneFileDir ):
    try:
        rootDir = PurePath( "C:/users/ltyler/Documents/Progression/assets/" )
        fullRelPath = PurePath( os.path.abspath( sceneFileDir + relPathStr ) )
        return fullRelPath.relative_to( rootDir ).as_posix().__str__()
    except:
        return relPathStr

def ParseLoaderResourcesIntoJson( loader, sceneDir ):
    resourceJson = []
    numInlinedModels = 0
    for tex in loader.scene.textures:
        json = {}
        json["name"] = tex.name
        json["filename"] = RelativeAssetPath( tex.params["filename"].value, sceneDir )

        combined = { "Image" : json }
        resourceJson.append( combined )

    for mat in loader.scene.materials:
        json = {}
        json["name"] = mat.id
        for key, val in mat.params.items():
            if val.type == 'texture':
                if val.name == "Kd":
                    json["albedoMap"] = val.value
                #else:
                #    print( "\t Texture " + val.name + ": " + val.value )
            else:
                if val.name == "Kd":
                    json["albedo"] = val.value
                #else:
                #    print( "\t", val.name, val.value )

        combined = { "Material": json }
        resourceJson.append( combined )

    # for shape in loader.scene.shapes:
    #     if shape.type == 'plymesh':
    #         json = {}
    #         fname = shape.params["filename"].value
    #         json["name"] = Path( fname ).stem
    #         json["filename"] = RelativeAssetPath( fname, sceneDir )

    #         combined = { "Model" : json }
    #         resourceJson.append( combined )
    #     elif shape.type == 'trianglemesh':
    #         numInlinedModels += 1
    #         modelName = "InlinedModel_" + str( numInlinedModels )
    #         print( "todo: output an obj for this inlined model" )
    
    return resourceJson

def ParseLoaderSceneIntoJson( loader, sceneDir ):
    sceneJson = []
    numInlinedModels = 0

    sensor = loader.scene.sensor
    width = sensor.film.params["xresolution"].value
    height = sensor.film.params["yresolution"].value
    aspectRatio = width / height
    vFov = 60
    if "halffov" in sensor.params:
        vFov = 2 * sensor.params["halffov"].value
    else:
        vFov = sensor.params["fov"].value

    # the fov in pbrt is the spread angle of the viewing frustum along the narrower of the image's width and height
    if height > width:
        vFov = 2 * atan( tan( vFov / 2.0 ) * height / width )

    json = {}
    json["aspectRatio"] = aspectRatio
    json["vFov"] = vFov
    if sensor.transform:
        matrix = sensor.transform.matrix
        position, rotation, scale = SeparateOutTransformMatrix( matrix )
        json["position"] = position
        #json["rotation"] = [ degrees( rotation[0] ), degrees( rotation[1] ), degrees( rotation[2] ) ]
        #json["scale"] = scale
    combined = { "Camera" : json }
    sceneJson.append( combined )
    
    for shape in loader.scene.shapes:
        modelName = ""
        if shape.type == 'plymesh':
            fname = shape.params["filename"].value
            modelName = Path( fname ).stem
        elif shape.type == 'trianglemesh':
            numInlinedModels += 1
            modelName = "InlinedModel_" + str( numInlinedModels )
            continue

        json = {}
        json["NameComponent"] = modelName
        json["ModelRenderer"] = { "model": modelName, "material": shape.material }
        combined = { "Entity" : json }
        sceneJson.append( combined )

    return sceneJson

def OutputJsonFile( resourceJson, filename ):
    # valid output, but 2x as many lines as needed (puts every brace, and every item of an array (ex: albedo color) on its own line)
    # resourceFile.write( pyJson.dumps( resourceJson, indent=4 ) )

    resourceFile = open( filename, "w" )
    resourceFile.write( "[\n" )
    numItems = len( resourceJson )
    itemsProcessed = 0
    for item in resourceJson:
        itemsProcessed += 1
        assetType = list(item.keys())[0]
        asset = item[assetType]
        resourceFile.write( "\t{ \"" + assetType + "\": {\n" )

        numAssetItems = len( asset )
        assetItemsProcessed = 0
        for k, v in asset.items():
            assetItemsProcessed += 1
            if type( v ) == str:
                resourceFile.write( "\t\t\"" + k + "\": \"" + v + "\"" )
            elif type( v ) == dict:
                resourceFile.write( "\t\t\"" + k + "\": " + pyJson.dumps( v ) )
            else:
                resourceFile.write( "\t\t\"" + k + "\": " + str( v ) )

            if assetItemsProcessed != numAssetItems:
                resourceFile.write( ",\n" )
            else:
                resourceFile.write( "\n" )

        if numItems != itemsProcessed:
            resourceFile.write( "\t} },\n" )
        else:
            resourceFile.write( "\t} }\n" )

    resourceFile.write( "]" )
    resourceFile.close


sceneFile = "../../assets/models/bathroom/scene.pbrt"
outputName = "bathroom"
sceneDir = PurePath( sceneFile ).parent.as_posix().__str__() + "/"
loader = PBRTv3Loader( sceneFile )

json = ParseLoaderResourcesIntoJson( loader, sceneDir )
OutputJsonFile( json, sceneDir + outputName + ".paf" )

json = ParseLoaderSceneIntoJson( loader, sceneDir )
OutputJsonFile( json, sceneDir + outputName + ".json" )