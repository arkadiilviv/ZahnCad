#include "zahn/surface.hpp"
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkTriangleFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkFillHolesFilter.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkDecimatePro.h>
#include <vtkStaticCellLocator.h>
#include <vtkCallbackCommand.h>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <cmath>
#include <limits>
#include <exception>
#include <bit>
#include <cstring>
#include <cstdint>
namespace zahn {
namespace {
void tick(const Checkpoint& check){if(check)check();}
void valid(Vec3 p){if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>1e8)throw std::runtime_error("Invalid surface coordinate");}
void validBox(Box box){valid(box.minimum);valid(box.maximum);if(box.minimum.x>=box.maximum.x||box.minimum.y>=box.maximum.y||box.minimum.z>=box.maximum.z)throw std::runtime_error("Box must have positive width, height and depth");}
struct PlyType{int bytes;bool floating,sign;};
PlyType plyType(const std::string& type){
  if(type=="char"||type=="int8")return {1,false,true};if(type=="uchar"||type=="uint8")return {1,false,false};
  if(type=="short"||type=="int16")return {2,false,true};if(type=="ushort"||type=="uint16")return {2,false,false};
  if(type=="int"||type=="int32")return {4,false,true};if(type=="uint"||type=="uint32")return {4,false,false};
  if(type=="float"||type=="float32")return {4,true,true};if(type=="double"||type=="float64")return {8,true,true};
  throw std::runtime_error("Unsupported PLY scalar type");
}
double plyScalar(std::istream& stream,PlyType type,bool ascii,bool little){
  double value=0;
  if(ascii){std::string token;if(!(stream>>token))throw std::runtime_error("Truncated PLY data");size_t pos;value=std::stod(token,&pos);if(pos!=token.size())throw std::runtime_error("Invalid PLY number");}
  else {unsigned char bytes[8]{};if(!stream.read(reinterpret_cast<char*>(bytes),type.bytes))throw std::runtime_error("Truncated binary PLY");if((std::endian::native==std::endian::little)!=little)std::reverse(bytes,bytes+type.bytes);
    if(type.floating){if(type.bytes==4){float number;std::memcpy(&number,bytes,4);value=number;}else std::memcpy(&value,bytes,8);}
    else if(type.sign){if(type.bytes==1){int8_t n;std::memcpy(&n,bytes,1);value=n;}else if(type.bytes==2){int16_t n;std::memcpy(&n,bytes,2);value=n;}else {int32_t n;std::memcpy(&n,bytes,4);value=n;}}
    else {if(type.bytes==1)value=bytes[0];else if(type.bytes==2){uint16_t n;std::memcpy(&n,bytes,2);value=n;}else {uint32_t n;std::memcpy(&n,bytes,4);value=n;}}
  }
  if(!std::isfinite(value))throw std::runtime_error("Non-finite PLY value");
  if(!type.floating){double low=type.sign?-std::ldexp(1.,type.bytes*8-1):0,high=std::ldexp(1.,type.sign?type.bytes*8-1:type.bytes*8)-1;if(std::floor(value)!=value||value<low||value>high)throw std::runtime_error("PLY integer out of range");}
  return value;
}
Mesh readPly(const std::filesystem::path& path,const Checkpoint& check){
  std::ifstream stream(path,std::ios::binary);std::string line;if(!std::getline(stream,line)||(line!="ply"&&line!="ply\r"))throw std::runtime_error("Invalid PLY header");
  struct Property{std::string name;PlyType type;bool list=false;PlyType count{};};struct Element{std::string name;size_t count;std::vector<Property> properties;};std::vector<Element> elements;
  bool ascii=false,little=true,formatSeen=false,ended=false;size_t lines=0;
  while(std::getline(stream,line)){tick(check);if(++lines>10000||line.size()>65536)throw std::runtime_error("Excessive PLY header");if(line=="end_header"||line=="end_header\r"){ended=true;break;}std::istringstream row(line);std::string word,name;row>>word;
    if(word=="format"){std::string version;row>>name>>version;if(formatSeen||version!="1.0"||(name!="ascii"&&name!="binary_little_endian"&&name!="binary_big_endian"))throw std::runtime_error("Unsupported PLY format");formatSeen=true;ascii=name=="ascii";little=name!="binary_big_endian";}
    else if(word=="element"){long long count=-1;row>>name>>count;if(count<0||count>30000000)throw std::runtime_error("Invalid PLY element count");elements.push_back({name,size_t(count),{}});}
    else if(word=="property"){if(elements.empty())throw std::runtime_error("PLY property without element");row>>name;Property property;if(name=="list"){std::string countType,valueType;row>>countType>>valueType>>property.name;property.list=true;property.count=plyType(countType);property.type=plyType(valueType);if(property.count.floating)throw std::runtime_error("PLY list count must be integer");}else{property.type=plyType(name);row>>property.name;}if(property.name.empty())throw std::runtime_error("Invalid PLY property");for(auto& p:elements.back().properties)if(p.name==property.name)throw std::runtime_error("Duplicate PLY property");elements.back().properties.push_back(property);}
    else if(!word.empty()&&word!="comment"&&word!="obj_info")throw std::runtime_error("Unsupported PLY header directive");
  }
  if(!ended||!formatSeen)throw std::runtime_error("Incomplete PLY header");bool x=false,y=false,z=false,r=false,g=false,b=false,vertexSeen=false,faceSeen=false;
  for(auto& element:elements){if(element.name=="vertex"){if(vertexSeen)throw std::runtime_error("Duplicate PLY vertex element");vertexSeen=true;for(auto& p:element.properties){if(p.list)continue;x|=p.name=="x";y|=p.name=="y";z|=p.name=="z";r|=p.name=="red";g|=p.name=="green";b|=p.name=="blue";}}if(element.name=="face")for(auto& p:element.properties)faceSeen|=p.list&&(p.name=="vertex_indices"||p.name=="vertex_index");}
  if(!x||!y||!z||!faceSeen)throw std::runtime_error("PLY requires vertex xyz and face indices");
  vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> cells;vtkNew<vtkUnsignedCharArray> rgb;rgb->SetName("RGB");rgb->SetNumberOfComponents(3);bool colored=r&&g&&b;
  for(auto& element:elements)for(size_t i=0;i<element.count;++i){if(i%1024==0)tick(check);Vec3 position;Color color{};std::vector<vtkIdType> face;
    for(auto& property:element.properties){if(property.list){double n=plyScalar(stream,property.count,ascii,little);if(n<0||n>10000)throw std::runtime_error("PLY list too large");bool indices=element.name=="face"&&(property.name=="vertex_indices"||property.name=="vertex_index");for(size_t j=0;j<size_t(n);++j){double v=plyScalar(stream,property.type,ascii,little);if(indices){if(v<0||v>30000000||std::floor(v)!=v)throw std::runtime_error("Invalid PLY face index");face.push_back(vtkIdType(v));}}}
      else{double value=plyScalar(stream,property.type,ascii,little);if(element.name!="vertex")continue;if(property.name=="x")position.x=value;else if(property.name=="y")position.y=value;else if(property.name=="z")position.z=value;else if(colored&&(property.name=="red"||property.name=="green"||property.name=="blue")){if(property.type.floating)value*=255;if(value<0||value>255)throw std::runtime_error("PLY RGB out of range");color[property.name=="red"?0:property.name=="green"?1:2]=static_cast<unsigned char>(std::lround(value));}}
    }
    if(element.name=="vertex"){valid(position);points->InsertNextPoint(position.x,position.y,position.z);if(colored)rgb->InsertNextTypedTuple(color.data());}
    if(element.name=="face"){if(face.size()<3)throw std::runtime_error("PLY face has fewer than three vertices");cells->InsertNextCell(vtkIdType(face.size()),face.data());}
  }
  vtkIdType n;const vtkIdType* ids;cells->InitTraversal();size_t checked=0;while(cells->GetNextCell(n,ids)){if(checked++%1024==0)tick(check);for(vtkIdType i=0;i<n;++i)if(ids[i]>=points->GetNumberOfPoints())throw std::runtime_error("PLY face references missing vertex");}
  vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetPolys(cells);if(colored)data->GetPointData()->SetScalars(rgb);return fromPolyData(data,check);
}
vtkSmartPointer<vtkPolyData> cleaned(const Mesh& input,const Checkpoint& check){
  vtkNew<vtkCleanPolyData> filter;filter->SetInputData(toPolyData(input,check));filter->ToleranceIsAbsoluteOn();filter->SetAbsoluteTolerance(0);
  filter->ConvertPolysToLinesOff();filter->ConvertLinesToPointsOff();filter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);updateAlgorithm(filter,check);
  auto out=vtkSmartPointer<vtkPolyData>::New();out->ShallowCopy(filter->GetOutput());return out;
}
Mesh subset(const Mesh& input,const std::vector<size_t>& selection,bool keep,const Checkpoint& check){
  std::vector<bool> chosen(input.triangles.size(),false);for(auto i:selection){if(i>=chosen.size())throw std::runtime_error("Selection belongs to a different mesh");chosen[i]=true;}
  Mesh out;for(size_t i=0;i<chosen.size();++i){if(i%1024==0)tick(check);if(chosen[i]==keep){out.triangles.push_back(input.triangles[i]);if(!input.colors.empty())out.colors.push_back(input.colors[i]);}}
  if(out.triangles.empty())throw std::runtime_error("Operation would remove the entire scan");return out;
}
}
void updateAlgorithm(vtkAlgorithm* filter,const Checkpoint& check){
  tick(check);struct Context {const Checkpoint* check;std::exception_ptr cancelled;bool error=false;};Context context{&check,{}};
  vtkNew<vtkCallbackCommand> callback;callback->SetClientData(&context);
  callback->SetCallback([](vtkObject* object,unsigned long event,void* data,void*){
    auto& c=*static_cast<Context*>(data);if(event==vtkCommand::ErrorEvent)c.error=true;
    if(*c.check)try{(*c.check)();}catch(...){c.cancelled=std::current_exception();static_cast<vtkAlgorithm*>(object)->SetAbortExecute(1);}
  });
  auto progress=filter->AddObserver(vtkCommand::ProgressEvent,callback);auto errors=filter->AddObserver(vtkCommand::ErrorEvent,callback);
  filter->Update();filter->RemoveObserver(progress);filter->RemoveObserver(errors);
  if(context.cancelled)std::rethrow_exception(context.cancelled);tick(check);if(context.error)throw std::runtime_error("Geometry operation rejected its input");
}
vtkSmartPointer<vtkPolyData> toPolyData(const Mesh& mesh,const Checkpoint& check){
  if(!mesh.colors.empty()&&mesh.colors.size()!=mesh.triangles.size())throw std::runtime_error("Invalid mesh color data");
  vtkNew<vtkPoints> points;points->SetDataTypeToDouble();points->SetNumberOfPoints(vtkIdType(mesh.triangles.size()*3));
  vtkNew<vtkCellArray> cells;cells->AllocateEstimate(vtkIdType(mesh.triangles.size()),3);
  vtkNew<vtkUnsignedCharArray> rgb;rgb->SetName("RGB");rgb->SetNumberOfComponents(3);if(!mesh.colors.empty())rgb->SetNumberOfTuples(points->GetNumberOfPoints());
  vtkIdType index=0;for(size_t i=0;i<mesh.triangles.size();++i){if(i%1024==0)tick(check);vtkIdType ids[3];for(int j=0;j<3;++j){auto p=mesh.triangles[i].v[j];valid(p);ids[j]=index;points->SetPoint(index,p.x,p.y,p.z);if(!mesh.colors.empty())rgb->SetTypedTuple(index,mesh.colors[i][j].data());++index;}cells->InsertNextCell(3,ids);}
  auto result=vtkSmartPointer<vtkPolyData>::New();result->SetPoints(points);result->SetPolys(cells);if(!mesh.colors.empty())result->GetPointData()->SetScalars(rgb);return result;
}
Mesh fromPolyData(vtkPolyData* data,const Checkpoint& check){
  vtkNew<vtkTriangleFilter> triangulate;triangulate->SetInputData(data);triangulate->PassLinesOff();triangulate->PassVertsOff();updateAlgorithm(triangulate,check);data=triangulate->GetOutput();
  if(data->GetNumberOfCells()==0||data->GetNumberOfCells()>10000000)throw std::runtime_error("Surface must contain 1 to 10 million triangles");
  Mesh result;result.triangles.reserve(size_t(data->GetNumberOfCells()));
  auto* rgb=vtkUnsignedCharArray::SafeDownCast(data->GetPointData()->GetScalars());bool colored=rgb&&rgb->GetNumberOfComponents()>=3;
  vtkIdType n=0;const vtkIdType* ids=nullptr;auto* cells=data->GetPolys();cells->InitTraversal();
  while(cells->GetNextCell(n,ids)){if(result.triangles.size()%1024==0)tick(check);if(n!=3)throw std::runtime_error("Triangulation failed");Triangle triangle;std::array<Color,3> colors;
    for(int j=0;j<3;++j){double p[3];data->GetPoint(ids[j],p);triangle.v[j]={p[0],p[1],p[2]};valid(triangle.v[j]);if(colored)for(int k=0;k<3;++k)colors[j][k]=static_cast<unsigned char>(rgb->GetComponent(ids[j],k));}
    result.triangles.push_back(triangle);if(colored)result.colors.push_back(colors);
  }return result;
}
Mesh readSurface(const std::filesystem::path& path,std::string format,const Checkpoint& check){
  tick(check);std::transform(format.begin(),format.end(),format.begin(),[](unsigned char c){return char(std::tolower(c));});
  if(format=="stl")return readStl(path,check);
  if(std::filesystem::file_size(path)>500000084)throw std::runtime_error("Surface file exceeds import limit");
  if(format=="ply")return readPly(path,check);
  if(format!="obj")throw std::runtime_error("Supported formats are STL, PLY and OBJ");
  std::ifstream stream(path);if(!stream)throw std::runtime_error("Cannot open OBJ");vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> cells;
  vtkNew<vtkUnsignedCharArray> rgb;rgb->SetName("RGB");rgb->SetNumberOfComponents(3);bool anyColor=false;std::string line;size_t lines=0,faces=0;
  while(std::getline(stream,line)){if(++lines%1024==0)tick(check);auto comment=line.find('#');if(comment!=std::string::npos)line.resize(comment);std::istringstream row(line);std::string word;row>>word;
    if(word=="v"){std::vector<double> values;double value;while(row>>value)values.push_back(value);if(!row.eof()||(values.size()!=3&&values.size()!=4&&values.size()!=6))throw std::runtime_error("OBJ vertices require xyz, xyzw or xyz RGB");
      Vec3 p{values[0],values[1],values[2]};if(values.size()==4){if(!std::isfinite(values[3])||values[3]==0)throw std::runtime_error("Invalid OBJ homogeneous coordinate");p={p.x/values[3],p.y/values[3],p.z/values[3]};}valid(p);points->InsertNextPoint(p.x,p.y,p.z);Color color{220,205,170};
      if(values.size()==6){anyColor=true;for(int i=0;i<3;++i){if(!std::isfinite(values[3+i])||values[3+i]<0||values[3+i]>1)throw std::runtime_error("OBJ RGB must be in the range 0 to 1");color[i]=static_cast<unsigned char>(std::lround(values[3+i]*255));}}rgb->InsertNextTypedTuple(color.data());if(points->GetNumberOfPoints()>30000000)throw std::runtime_error("Too many OBJ vertices");
    }else if(word=="f"){std::vector<vtkIdType> face;std::string vertex;while(row>>vertex){auto slash=vertex.find('/');auto text=vertex.substr(0,slash);size_t parsed;long long id=std::stoll(text,&parsed);if(parsed!=text.size()||id==0)throw std::runtime_error("Invalid OBJ face index");auto resolved=id>0?id-1:points->GetNumberOfPoints()+id;if(resolved<0||resolved>=points->GetNumberOfPoints())throw std::runtime_error("OBJ face references missing vertex");face.push_back(resolved);if(face.size()>10000)throw std::runtime_error("OBJ polygon too large");}if(face.size()<3)throw std::runtime_error("OBJ face has fewer than three vertices");cells->InsertNextCell(vtkIdType(face.size()),face.data());if(++faces>10000000)throw std::runtime_error("Too many OBJ faces");}
  }
  vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetPolys(cells);if(anyColor)data->GetPointData()->SetScalars(rgb);return fromPolyData(data,check);
}
std::vector<size_t> selectBox(const Mesh& mesh,Box box,const Checkpoint& check){
  validBox(box);std::vector<size_t> result;for(size_t i=0;i<mesh.triangles.size();++i){if(i%1024==0)tick(check);auto& v=mesh.triangles[i].v;Vec3 p{(v[0].x+v[1].x+v[2].x)/3,(v[0].y+v[1].y+v[2].y)/3,(v[0].z+v[1].z+v[2].z)/3};if(p.x>=box.minimum.x&&p.x<=box.maximum.x&&p.y>=box.minimum.y&&p.y<=box.maximum.y&&p.z>=box.minimum.z&&p.z<=box.maximum.z)result.push_back(i);}return result;
}
Mesh processSurface(const Mesh& input,const Parameters& params,const Checkpoint& check){
  tick(check);if(input.triangles.empty())throw std::runtime_error("Select a nonempty scan");
  if(!params.selectedVertices.empty()){
    std::set<std::array<double,3>> vertices;for(auto p:params.selectedVertices){valid(p);vertices.insert({p.x,p.y,p.z});}
    Parameters resolved=params;resolved.selected.clear();resolved.selectedVertices.clear();
    for(size_t i=0;i<input.triangles.size();++i){if(i%1024==0)tick(check);int count=0;for(auto p:input.triangles[i].v)count+=vertices.contains({p.x,p.y,p.z});
      if((params.operation==Operation::KeepSelected&&count==3)||(params.operation!=Operation::KeepSelected&&count>0))resolved.selected.push_back(i);
    }
    if(params.operation==Operation::SelectedComponent&&!resolved.selected.empty())resolved.selected.resize(1);
    if(resolved.selected.empty())throw std::runtime_error("No complete selected triangles. Paint all three vertices to keep a triangle.");
    return processSurface(input,resolved,check);
  }
  if(params.operation==Operation::KeepSelected||params.operation==Operation::DeleteSelected){if(params.selected.empty())throw std::runtime_error("Select faces first");return subset(input,params.selected,params.operation==Operation::KeepSelected,check);}
  if(params.operation==Operation::Crop){validBox(params.box);Mesh output;bool colored=!input.colors.empty();
    struct Vertex{Vec3 p;Color color;};
    for(size_t i=0;i<input.triangles.size();++i){if(i%1024==0)tick(check);std::vector<Vertex> polygon;for(int j=0;j<3;++j)polygon.push_back({input.triangles[i].v[j],colored?input.colors[i][j]:Color{}});
      for(int side=0;side<6&&!polygon.empty();++side){int axis=side/2;double bound=axis==0?(side%2?params.box.maximum.x:params.box.minimum.x):axis==1?(side%2?params.box.maximum.y:params.box.minimum.y):(side%2?params.box.maximum.z:params.box.minimum.z);
        auto distance=[&](Vertex v){double coordinate=axis==0?v.p.x:axis==1?v.p.y:v.p.z;return side%2?bound-coordinate:coordinate-bound;};std::vector<Vertex> next;
        auto previous=polygon.back();double before=distance(previous);
        for(auto current:polygon){double after=distance(current);if((after>=0)!=(before>=0)){double t=before/(before-after);Vertex intersection;intersection.p={previous.p.x+t*(current.p.x-previous.p.x),previous.p.y+t*(current.p.y-previous.p.y),previous.p.z+t*(current.p.z-previous.p.z)};for(int k=0;k<3;++k)intersection.color[k]=static_cast<unsigned char>(std::clamp(std::lround(previous.color[k]+t*(double(current.color[k])-previous.color[k])),0l,255l));next.push_back(intersection);}if(after>=0)next.push_back(current);previous=current;before=after;}polygon=std::move(next);
      }
      for(size_t j=1;j+1<polygon.size();++j){auto a=polygon[0].p,b=polygon[j].p,c=polygon[j+1].p;Vec3 u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};if(std::hypot(u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x)<=1e-16)continue;output.triangles.push_back({{a,b,c}});if(colored)output.colors.push_back({polygon[0].color,polygon[j].color,polygon[j+1].color});if(output.triangles.size()>10000000)throw std::runtime_error("Crop result exceeds triangle limit");}
    }if(output.triangles.empty())throw std::runtime_error("Crop box does not intersect the scan");return output;
  }
  auto data=cleaned(input,check);
  if(params.operation==Operation::Smooth||params.operation==Operation::Simplify||params.operation==Operation::FillHoles){auto topology=inspect(input,check);if(topology.nonManifoldEdges||topology.degenerate||topology.inconsistentEdges)throw std::runtime_error("Clean degenerate/non-manifold geometry and correct winding before this operation");}
  if(params.operation==Operation::LargestComponent||params.operation==Operation::SelectedComponent){vtkNew<vtkPolyDataConnectivityFilter> filter;filter->SetInputData(data);filter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    if(params.operation==Operation::LargestComponent)filter->SetExtractionModeToLargestRegion();else{if(params.selected.size()!=1||params.selected[0]>=input.triangles.size())throw std::runtime_error("Select exactly one face on the component first");auto p=input.triangles[params.selected[0]].v[0];filter->SetExtractionModeToClosestPointRegion();filter->SetClosestPoint(p.x,p.y,p.z);}updateAlgorithm(filter,check);return fromPolyData(filter->GetOutput(),check);}
  if(params.operation==Operation::Clean){
    Mesh out=fromPolyData(data,check),result;using Point=std::array<double,3>;using Face=std::array<Point,3>;std::set<Face> seen;
    for(size_t i=0;i<out.triangles.size();++i){if(i%1024==0)tick(check);auto v=out.triangles[i].v;Vec3 a{v[1].x-v[0].x,v[1].y-v[0].y,v[1].z-v[0].z},b{v[2].x-v[0].x,v[2].y-v[0].y,v[2].z-v[0].z};double area=std::hypot(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);if(area<=1e-16)continue;Face key{{{v[0].x,v[0].y,v[0].z},{v[1].x,v[1].y,v[1].z},{v[2].x,v[2].y,v[2].z}}};std::sort(key.begin(),key.end());if(seen.insert(key).second){result.triangles.push_back(out.triangles[i]);if(!out.colors.empty())result.colors.push_back(out.colors[i]);}}
    if(result.triangles.empty())throw std::runtime_error("No valid faces remain after cleaning");return result;
  }
  if(params.operation==Operation::Orient){vtkNew<vtkPolyDataNormals> filter;filter->SetInputData(data);filter->ConsistencyOn();filter->SplittingOff();filter->AutoOrientNormalsOff();updateAlgorithm(filter,check);return fromPolyData(filter->GetOutput(),check);}
  if(params.operation==Operation::FillHoles){if(!std::isfinite(params.value)||params.value<=0||params.value>50)throw std::runtime_error("Hole radius must be between 0 and 50 mm");vtkNew<vtkFillHolesFilter> filter;filter->SetInputData(data);filter->SetHoleSize(params.value);updateAlgorithm(filter,check);return fromPolyData(filter->GetOutput(),check);}
  if(params.operation==Operation::Smooth){if(params.iterations<1||params.iterations>100||!std::isfinite(params.value)||params.value<=0||params.value>0.5)throw std::runtime_error("Invalid smoothing parameters");vtkNew<vtkSmoothPolyDataFilter> filter;filter->SetInputData(data);filter->SetNumberOfIterations(params.iterations);filter->SetRelaxationFactor(params.value);filter->BoundarySmoothingOff();filter->FeatureEdgeSmoothingOff();filter->SetConvergence(0);filter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);updateAlgorithm(filter,check);return fromPolyData(filter->GetOutput(),check);}
  if(params.operation==Operation::Simplify){if(!std::isfinite(params.value)||params.value<=0||params.value>=1)throw std::runtime_error("Reduction must be between 0 and 1");vtkNew<vtkDecimatePro> filter;filter->SetInputData(data);filter->SetTargetReduction(params.value);filter->PreserveTopologyOn();filter->BoundaryVertexDeletionOff();filter->SplittingOff();filter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);updateAlgorithm(filter,check);return fromPolyData(filter->GetOutput(),check);}
  throw std::runtime_error("Unsupported surface operation");
}
Deviation deviation(const Mesh& before,const Mesh& after,const Checkpoint& check){
  if(before.triangles.empty()||after.triangles.empty())throw std::runtime_error("Deviation requires two nonempty surfaces");Deviation result;double squared=0;
  auto measure=[&](const Mesh& from,const Mesh& to){vtkNew<vtkStaticCellLocator> locator;locator->SetDataSet(toPolyData(to,check));locator->BuildLocator();tick(check);size_t stride=std::max(size_t(1),(from.triangles.size()+24999)/25000);
    for(size_t i=0;i<from.triangles.size();i+=stride){tick(check);auto v=from.triangles[i].v;Vec3 center{(v[0].x+v[1].x+v[2].x)/3,(v[0].y+v[1].y+v[2].y)/3,(v[0].z+v[1].z+v[2].z)/3};for(auto p:{v[0],v[1],v[2],center}){double point[3]{p.x,p.y,p.z},closest[3]{},distance=std::numeric_limits<double>::infinity();vtkIdType cell=-1;int sub=-1;locator->FindClosestPoint(point,closest,cell,sub,distance);if(cell<0||!std::isfinite(distance)||distance<0)throw std::runtime_error("Deviation query failed");squared+=distance;result.maximum=std::max(result.maximum,std::sqrt(distance));++result.samples;}}
  };measure(before,after);measure(after,before);result.rms=std::sqrt(squared/double(result.samples));return result;
}
}
