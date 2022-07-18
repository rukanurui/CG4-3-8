#include "FbxLoader.h"


const std::string FbxLoader::baseDirectory = "Resources/";
const std::string FbxLoader::defaultTextureFileName = "white1x1.png";

using namespace DirectX;

FbxLoader* FbxLoader::GetInstance()
{
    static FbxLoader instance;
    return &instance;
}

void FbxLoader::Initialize(ID3D12Device* device)
{
    //再初期化チェック
    assert(fbxManager == nullptr);
    //因数からメンバ変数に代入
    this->device = device;
    //FBXマネージャの生成
    fbxManager = FbxManager::Create();
    //FBXマネージャの入出力設定
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);
    //FBXインポータの生成
    fbxImporter = FbxImporter::Create(fbxManager,"");
}

void FbxLoader::Finalize()
{
    //各種FBXインスタンスの破棄
    fbxImporter->Destroy();
    fbxManager->Destroy();
}

void FbxLoader::ParseSkin(FbxModel* fbxmodel, FbxMesh* fbxMesh)
{
    //スキニング情報
    FbxSkin* fbxSkin =
        static_cast<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::eSkin));
    //スキニング情報がなければ終了
    if (fbxSkin == nullptr) {
        ////各頂点について処理
        //for (int i = 0; i < fbxmodel->vertices.size(); i++)
        //{
        //    //最初のボーン(単に行列)の影響100%にする
        //    fbxmodel->vertices[i].boneIndex[0] = 0;
        //    fbxmodel->vertices[i].boneweight[0] = 1.0f;
        //}
        return;
    }
    //ボーン配列の参照
    std::vector<FbxModel::Bone>& bones = fbxmodel->bones;

    //ボーンの数
    int clusterCount = fbxSkin->GetClusterCount();
    bones.reserve(clusterCount);
    //全てのボーンについて
    for (int i = 0; i < clusterCount; i++) {
        //Fbxボーン情報
        FbxCluster* fbxCluster = fbxSkin->GetCluster(i);

        //ボーン自体のノードの名前を取得
        const char* boneName = fbxCluster->GetLink()->GetName();

        //新しくボーンを追加し、追加したボーンの参照を得る
        bones.emplace_back(FbxModel::Bone(boneName));
        FbxModel::Bone& bone = bones.back();
        //自作ボーンとFbxのボーンを紐づける
        bone.fbxCluster = fbxCluster;

        //FBXから徐貴志政行列を取得する
        FbxAMatrix fbxMat;
        fbxCluster->GetTransformLinkMatrix(fbxMat);
        //XMMatrix型に変換する
        XMMATRIX initialPose;
        ConvertMatrixFromFbx(&initialPose, fbxMat);
        //初期姿勢の逆行列を得る
        bone.invInitialPose = XMMatrixInverse(nullptr, initialPose);

    }
    //ボーン番号とスキンウェイトのペア
    struct WeightSet {
        UINT index;
        float weight;
    };
    //二次元配列(ジャグ配列)
    //List:頂点が影響を受けるボーンの全リスト
    //vector:それを全頂点分
    std::vector<std::list<WeightSet>>
        WeightLists(fbxmodel->vertices.size());
    //全てのボーンについて
    for (int i = 0; i < clusterCount; i++) {
        //FBXボーン情報
        FbxCluster* fbxCluster = fbxSkin->GetCluster(i);
        //子のボーンに影響を受ける頂点の数
        int controlPointIndicesCount = fbxCluster->GetControlPointIndicesCount();
        //子のボーンに影響を受ける頂点の配列
        int* controlPointIndices = fbxCluster->GetControlPointIndices();
        double* controlPointWeights = fbxCluster->GetControlPointWeights();
        //影響を受ける全頂点について
        for (int j = 0; j < controlPointIndicesCount; j++)
        {
            //頂点番号
            int vertIndex = controlPointIndices[j];
            //スキンウェイト
            float weight = (float)controlPointWeights[j];
            //その頂点の影響を受けるボーンリストに、ボーンとウェイトのペアを追加
            WeightLists[vertIndex].emplace_back(WeightSet{ (UINT)i,weight });
        }
    }
    //頂点書き換え用の参照
    auto& vertices = fbxmodel->vertices;
    //各頂点について処理
    for (int i = 0; i < vertices.size(); i++)
    {
        //頂点のウェイトから最も大きい4つを選択
        auto& weightList = WeightLists[i];
        //大証皮革用のラムダ式を指定して降順にソート
        weightList.sort([](auto const& lhs, auto const& rhs) {
            //左の要素のほうが大きければtrue そうでなければfalseを返す
            return lhs.weight > rhs.weight;
            });
        int weightArrayIndex = 0;
        //降順ソート済みのウェイトリストから
        for (auto& weightSet : weightList) {
            //頂点データに書き込み
            vertices[i].boneIndex[weightArrayIndex] = weightSet.index;
            vertices[i].boneweight[weightArrayIndex] = weightSet.weight;
            //4つに達したら終了
            if (++weightArrayIndex >= FbxModel::MAX_BONE_INDICES) {
                float weight = 0.0f;
                //2番目以降のウェイトを合計
                for (int j = 0; j < FbxModel::MAX_BONE_INDICES; j++) {
                    weight += vertices[i].boneweight[j];
                }
                //合計で1.0f(100%)になるように調整
                vertices[i].boneweight[0] = 1.0f - weight;
                break;
            }
        }
    }
}

FbxModel*FbxLoader::LoadModelFromFile(const string& modelName)
{
    //モデルと同じ名前のフォルダから読み込む
    const string directorypath = baseDirectory + modelName + "/";
    //拡張子、fbxを付加
    const string fileName = modelName + ".fbx";
    //連結してフルパスを得る
    const string fullpath = directorypath + fileName;
    //ファイル名を指定してFBXファイルを読み込む
    if (!fbxImporter->Initialize(fullpath.c_str(), -1, fbxManager->GetIOSettings())) {
        assert(0);
    }

    //シーン生成
    FbxScene* fbxScene =
        FbxScene::Create(fbxManager, "fbxScene");
    //ファイルからロードしたFBXの情報をシーンにインポート
    fbxImporter->Import(fbxScene);

    //モデル生成
    FbxModel* fbxmodel = new FbxModel();
    fbxmodel->name = modelName;

    //FBXノードの数を取得
    int nodeCount = fbxScene->GetNodeCount();
    //あらかじめ必要数分のメモリを確保することで、アドレスがずれるのを予防
    fbxmodel->nodes.reserve(nodeCount);

    //ルートノードから順に解析してモデルに組み込む
    ParseNodeRecursive(fbxmodel, fbxScene->GetRootNode());

    //バッファ生成
    fbxmodel->CreateBuffers(device);

    //fbxシーン解放
    fbxmodel->fbxScene = fbxScene;

    return fbxmodel;

   

}

void FbxLoader::ParseNodeRecursive(FbxModel* model, FbxNode* fbxNode, Node* parent)
{
    //ノード名を取得
    string name = fbxNode->GetName();

    //モデルにノードを追加
    model->nodes.emplace_back();
    Node& node = model->nodes.back();
    //ノード名を取得
    node.name = fbxNode->GetName();

    //FBXノードのローカル移動情報
    FbxDouble3 rotation = fbxNode->LclRotation.Get();
    FbxDouble3 scaling = fbxNode->LclScaling.Get();
    FbxDouble3 translation = fbxNode->LclTranslation.Get();

    //形式変換して代入
    node.rotation = { (float)rotation[0],(float)rotation[1], (float)rotation[2], 0.0f };
    node.rotation = { (float)scaling[0],(float)scaling[1], (float)scaling[2], 0.0f };
    node.translation = { (float)translation[0],(float)translation[1], (float)translation[2], 0.0f };

    //回転角をDegree(度)からラジアンに変換
    node.rotation.m128_f32[0] = XMConvertToRadians(node.rotation.m128_f32[0]);
    node.rotation.m128_f32[1] = XMConvertToRadians(node.rotation.m128_f32[1]);
    node.rotation.m128_f32[2] = XMConvertToRadians(node.rotation.m128_f32[2]);

    //スケール、回転、平行移動行列の計算
    XMMATRIX matScaling, matRotation, matTranslation;
    matScaling = XMMatrixScalingFromVector(node.scaling);
    matRotation = XMMatrixRotationRollPitchYawFromVector(node.rotation);
    matTranslation = XMMatrixTranslationFromVector(node.translation);

    //ローカル変形行列の計算
    node.transform = XMMatrixIdentity();
    node.transform *= matScaling;
    node.transform *= matRotation;
    node.transform *= matTranslation;

    //グローバル変形行列の計算
    node.globalTransform = node.transform;
    if (parent) {
        node.parent = parent;
        //親の変形を乗算
        node.globalTransform *= parent->globalTransform;
    }

    //FBXノードのメッシュ情報を解析
    FbxNodeAttribute* fbxNodeAttribute = fbxNode->GetNodeAttribute();

    if (fbxNodeAttribute) {
        if (fbxNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh) {
            model->meshNode = &node;
            ParseMesh(model, fbxNode);
        }
    }

    //子ノードに対して再帰呼び出し
    for (int i = 0; i < fbxNode->GetChildCount(); i++) {
        ParseNodeRecursive(model, fbxNode->GetChild(i), &node);
    }

}

void FbxLoader::ParseMesh(FbxModel* model, FbxNode* fbxNode)
{
    //ノードのメッシュを取得
    FbxMesh* fbxMesh = fbxNode->GetMesh();

    //頂点座標読み取り
    ParseMeshVertices(model, fbxMesh);
    //面を構成するデータの読み取り
    ParceMeshFaces(model, fbxMesh);
    //マテリアルの読み取り
    ParseMaterial(model, fbxNode);
    //スキニング情報の読み取り
    ParseSkin(model, fbxMesh);
}

void FbxLoader::ParseMeshVertices(FbxModel* model, FbxMesh* fbxMesh)
{
    auto& vertices = model->vertices;

    //頂点座標データの数
    const int contorolPointsCount =
        fbxMesh->GetControlPointsCount();
    //必要数だけ頂点データ配列を確保
    /*FbxModel::VertexPosNormalUVSkin vert{};
    model->vertices.resize(contorolPointsCount, vert);*/

    FbxModel::VertexPosNormalUVSkin vert{};
    model->vertices.resize(contorolPointsCount, vert);

    //fbxメッシュの頂点座標配列を取得
    FbxVector4* pCoord = fbxMesh->GetControlPoints();

    ////fbxメッシュの頂点座標をモデル内の配列にコピーする
    //for (int i = 0; i < contorolPointsCount; i++)
    //{
    //    FbxModel::VertexPosNormalUVSkin& vertex = vertices[i];
    //    //座標のコピー
    //    vertex.pos.x = (float)pCoord[i][0];
    //    vertex.pos.y = (float)pCoord[i][1];
    //    vertex.pos.z = (float)pCoord[i][2];
    //}
     //fbxメッシュの頂点座標をモデル内の配列にコピーする
    for (int i = 0; i < contorolPointsCount; i++)
    {
        FbxModel::VertexPosNormalUVSkin& vertex = vertices[i];
        //座標のコピー
        vertex.pos.x = (float)pCoord[i][0];
        vertex.pos.y = (float)pCoord[i][1];
        vertex.pos.z = (float)pCoord[i][2];
    }
}

void FbxLoader::ParceMeshFaces(FbxModel* model, FbxMesh* fbxMesh)
{
    auto& vertices = model->vertices;
    auto& indices = model->indices;

    //1ファイルに複数メッシュのモデルは非対応
    assert(indices.size() == 0);
    //面の数
    const int polygonCount = fbxMesh->GetPolygonCount();
    //UVデータの数
    const int textureUVCount = fbxMesh->GetTextureUVCount();
    //UV名リスト
    FbxStringList uvNames;
    fbxMesh->GetUVSetNames(uvNames);
    //面ごとの情報読み取り
    for (int i = 0; i < polygonCount; i++)
    {
        //面を構成する頂点の数を取得(3なら三角形ポリゴン)
        const int polygonSize = fbxMesh->GetPolygonSize(i);
        assert(polygonSize <= 4);

        //1頂点ずつ処理
        for (int j = 0; j < polygonSize; j++)
        {
            //FBX頂点配列のインデックス
            int index = fbxMesh->GetPolygonVertex(i, j);
            assert(index >= 0);

            //頂点法線読み込み
            FbxModel::VertexPosNormalUVSkin& vertex = vertices[index];
            FbxVector4 normal;
            if (fbxMesh->GetPolygonVertexNormal(i, j, normal))
            {
                vertex.normal.x = (float)normal[0];
                vertex.normal.y = (float)normal[1];
                vertex.normal.z = (float)normal[2];
            }

            //テクスチャ読み込み
            if (textureUVCount > 0)
            {
                FbxVector2 uvs;
                bool lUnmappedUV;
                //0番決め打ちで読み込み
                if (fbxMesh->GetPolygonVertexUV(i, j, uvNames[0], uvs, lUnmappedUV))
                {
                    vertex.uv.x = (float)uvs[0];
                    vertex.uv.x = (float)uvs[1];
                }
            }
            if (j < 3)
            {
                //1点追加し、他の2点と三角形を構築する
                indices.push_back(index);
            }
            //4頂点目
            else {
                //3点追加し、
                //四角形の0,1,2,3農地　2,3,0で三角形を構築する
                int index2 = indices[indices.size() - 1];
                int index3 = index;
                int index0 = indices[indices.size() - 3];
                indices.push_back(index2);
                indices.push_back(index3);
                indices.push_back(index0);
            }
        }
    }
}

void FbxLoader::ParseMaterial(FbxModel* model, FbxNode* fbxNode)
{
    const int materialCount = fbxNode->GetMaterialCount();
    if (materialCount > 0)
    {
        //先頭のマテリアルを取得
        FbxSurfaceMaterial* material = fbxNode->GetMaterial(0);
        //テクスチャを読み込んだかどうかを表すフラグ
        bool textureLoaded = false;

        if (material)
        {
            if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
            {
                FbxSurfaceLambert* lambert =
                    static_cast<FbxSurfaceLambert*>(material);

                //環境光係数
                FbxPropertyT<FbxDouble3> ambient = lambert->Ambient;
                model->ambient.x = (float)ambient.Get()[0];
                model->ambient.y = (float)ambient.Get()[1];
                model->ambient.z = (float)ambient.Get()[2];

                //拡散反射光係数
                FbxPropertyT<FbxDouble3> diffuse = lambert->Diffuse;
                model->diffuse.x = (float)diffuse.Get()[0];
                model->diffuse.y = (float)diffuse.Get()[1];
                model->diffuse.z = (float)diffuse.Get()[2];

                const FbxProperty diffuseproperty =
                    material->FindProperty(FbxSurfaceMaterial::sDiffuse);
                if (diffuseproperty.IsValid())
                {
                    const FbxFileTexture* texture = diffuseproperty.GetSrcObject<FbxFileTexture>();
                    if (texture)
                    {
                        const char* filepath = texture->GetFileName();
                        //ファイルぱすからファイル名抽出
                        string path_str(filepath);
                        string name = ExtractFileName(path_str);
                        //テクスチャ読み込み
                        LoadTexture(model, baseDirectory + model->name + "/" + name);
                        textureLoaded = true;
                    }
                }
            }
        }
        //テクスチャがない場合はテクスチャを張る
        if (!textureLoaded)
        {
            LoadTexture(model, baseDirectory + defaultTextureFileName);
        }
    }
}

void FbxLoader::LoadTexture(FbxModel* model, const std::string& fullpath)
{
    HRESULT result = S_FALSE;
    //WICテクスチャのロード
    TexMetadata& metadata = model->metadeta;
    ScratchImage& scratchImg = model->scratchImg;
    //ユニコード文字列に変換
    wchar_t wfilepath[128];
    MultiByteToWideChar(CP_ACP, 0, fullpath.c_str(), -1, wfilepath, _countof(wfilepath));
    result = LoadFromWICFile(
        wfilepath, WIC_FLAGS_NONE,
        &metadata, scratchImg);
    if (FAILED(result)) {
        assert(0);
    }
}

std::string FbxLoader::ExtractFileName(const std::string& path)
{
    size_t pos1;
    //区切り文字"\\"が出てくる一番最後の部分を探索
    pos1 = path.rfind('\\');
    if (pos1 != string::npos) {
        return path.substr(pos1 + 1, path.size() - pos1 - 1);
    }
    //区切り文字'/'が出てくる一番最後の部分を検索
    pos1 = path.rfind('/');
    if (pos1 != string::npos)
    {
        return path.substr(pos1 + 1, path.size() - pos1 - 1);
    }
    return path;
}

void FbxLoader::ConvertMatrixFromFbx(DirectX::XMMATRIX* dst, const FbxMatrix& src)
{
    //行
    for (int i = 0; i < 4; i++)
    {
        //列
        for (int j = 0; j < 4; j++)
        {
            //1要素コピー
            dst->r[i].m128_f32[j] = (float)src.Get(i, j);
        }
    }
}