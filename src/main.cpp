// Copyright 2025 Greenbox
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
sf::RenderTexture rt({160, 144});
auto window = sf::RenderWindow(sf::VideoMode({160, 144}), "The game trademark");
char right = 0, down = 0, up = 0, left = 0, confirm = 0, cancel = 0, select = 0,start = 0;
sf::Keyboard::Key rightkey = sf::Keyboard::Key::Right,
                  downkey = sf::Keyboard::Key::Down,
                  upkey = sf::Keyboard::Key::Up,
                  leftkey = sf::Keyboard::Key::Left,
                  confirmkey = sf::Keyboard::Key::Z,
                  cancelkey = sf::Keyboard::Key::X,
                  startkey = sf::Keyboard::Key::Enter,
                  selectkey = sf::Keyboard::Key::C;
sf::View view({0.f, 0.f}, {160.f, 144.f});
bool groundcheck=false,doublejump=true,dash=true,floating=false;
std::unordered_map<std::string,sf::Texture> texturemap;

float Lerp(float A, float B, float Alpha) {
  return A * (1 - Alpha) + B * Alpha;
}


struct ground{
    int x,y,x2,y2;
};
std::vector<ground> groundvector;
struct entity{
    int x=0,y=0;
    float xvelocity=0,yvelocity=0;
    int vertx=-8,verty=-1,vertx2=8,verty2=0;
    int hitboxx1=-8,hitboxy1=-16,hitboxx2=8,hitboxy2=0;
    int anim=0;
};
entity player;

bool overlap(entity p,ground g)
{
   if (p.vertx+p.x >= g.x+g.x2 || g.x >= p.vertx2+p.x )
        return false;

    if (p.verty2+p.y  <= g.y || g.y2+g.y <= p.verty+p.y)
        return false;

    return true;
}

void windowset(){
  while (std::optional event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window.close();
    }

    if (auto resized = event->getIf<sf::Event::Resized>()) {
      float x, y;
      if ((resized->size.x) > (resized->size.y)) {
        y = 144;
        x = (float(resized->size.x) / float(resized->size.y) * 144.f);
      } else {
        x = 160;
        y = (float(resized->size.y) / float(resized->size.x) * 160.f);
      }
      sf::FloatRect visibleArea({(-x + 160.f) / 2, (-y + 144.f) / 2}, {x, y});

      window.setView(sf::View(visibleArea));
    }
  }
}

void keypresscheck(sf::Keyboard::Key keycode, char* key) {
  if (sf::Keyboard::isKeyPressed(keycode)) {
    if (*key == 0)
      *key = 2;
    else if (*key == 2)
      *key = 1;
  } else {
    *key = 0;
  }
}

void collisioncheck(){
  groundcheck=false;
  player.y++;
  for(int i=0;i<groundvector.size();i++){
    if(overlap(player,groundvector[i])){
      groundcheck=true;
      while(overlap(player,groundvector[i]))player.y--;
      break;
      }
  }
  if(!groundcheck)player.y--;
}


void update() {
  view.setCenter({Lerp(view.getCenter().x,float(player.x+64),0.5f),-64});
  
  collisioncheck();

  if(groundcheck){player.yvelocity=0;doublejump=true;}//더블 점프 활성화 + 땅에 있을시에 y가속도 0으로 설정
  else player.yvelocity+=0.5f*(floating&&player.yvelocity>0?0.125f:1);//중력

  if(player.xvelocity>2)player.xvelocity-=0.5f;

  if(player.xvelocity>2)player.xvelocity-=0.5;
  if(player.xvelocity<2)player.xvelocity=2;
  if(player.xvelocity==2&&groundcheck)dash=true;
  if(confirm==2){//점프
    floating=false;
    if(groundcheck)player.yvelocity=-7;
    else if(doublejump){doublejump=false;player.yvelocity=-7;}
  }
  else if(confirm==1){
    floating=true;
  }
  else floating=false;
  if(select==2&&dash){
    floating=false;
    dash=false;
    player.xvelocity=10;
  }
  collisioncheck();
  player.x+=player.xvelocity;
  if(player.yvelocity<0)player.y+=player.yvelocity;
  else for(int i=0;i<player.yvelocity;i++){
    player.y++;
    collisioncheck();
    if(groundcheck){player.yvelocity=0;doublejump=true;}
  }
}

void input(){
  keypresscheck(rightkey,&right);
  keypresscheck(leftkey,&left);
  keypresscheck(upkey,&up);
  keypresscheck(downkey,&down);
  keypresscheck(confirmkey,&confirm);
  keypresscheck(cancelkey,&cancel);
  keypresscheck(startkey,&start);
  keypresscheck(selectkey,&select);
}

void render() {
  rt.setView(view);
  window.clear();
  rt.clear();
  sf::VertexArray tri(sf::PrimitiveType::TriangleStrip, 4);
  for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
  tri[0].position=sf::Vector2f(player.x+64-128,-120-64);
  tri[1].position=sf::Vector2f(player.x+64+128,-120-64);
  tri[2].position=sf::Vector2f(player.x+64-128,120-64);
  tri[3].position=sf::Vector2f(player.x+64+128,120-64);
  tri[0].texCoords=sf::Vector2f(0,0);
  tri[1].texCoords=sf::Vector2f(256,0);
  tri[2].texCoords=sf::Vector2f(0,240);
  tri[3].texCoords=sf::Vector2f(256,240);
  rt.draw(tri,&texturemap["Background1"]);
  for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
  for(int i=0;i<groundvector.size();i++){
    tri[0].position=sf::Vector2f(groundvector[i].x,groundvector[i].y);
    tri[1].position=sf::Vector2f(groundvector[i].x+groundvector[i].x2,groundvector[i].y);
    tri[2].position=sf::Vector2f(groundvector[i].x,groundvector[i].y+groundvector[i].y2);
    tri[3].position=sf::Vector2f(groundvector[i].x+groundvector[i].x2,groundvector[i].y+groundvector[i].y2);
    rt.draw(tri);
  }
  for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
  tri[0].position=sf::Vector2f(player.x+player.hitboxx1,player.y+player.hitboxy1);
  tri[1].position=sf::Vector2f(player.x+player.hitboxx2,player.y+player.hitboxy1);
  tri[2].position=sf::Vector2f(player.x+player.hitboxx1,player.y+player.hitboxy2);
  tri[3].position=sf::Vector2f(player.x+player.hitboxx2,player.y+player.hitboxy2);
  player.anim++;
  if(player.anim>39)player.anim=0;
  int anim=0;
  switch(player.anim/10){
    case 0:case 2:anim=0;break;
    case 1:anim=1;break;
    case 3:anim=2;break;
  }
  tri[0].texCoords=sf::Vector2f((anim)*(player.hitboxx2-player.hitboxx1),0);
  tri[1].texCoords=sf::Vector2f((1+anim)*(player.hitboxx2-player.hitboxx1),0);
  tri[2].texCoords=sf::Vector2f((anim)*(player.hitboxx2-player.hitboxx1),player.hitboxy2-player.hitboxy1);
  tri[3].texCoords=sf::Vector2f((1+anim)*(player.hitboxx2-player.hitboxx1),player.hitboxy2-player.hitboxy1);
  rt.draw(tri,&texturemap["Player"]);
  // for(int i=0;i<4;i++)tri[i].color=sf::Color::Green;
  // tri[0].position=sf::Vector2f(player.x+player.vertx,player.y+player.verty);
  // tri[1].position=sf::Vector2f(player.x+player.vertx2,player.y+player.verty);
  // tri[2].position=sf::Vector2f(player.x+player.vertx,player.y+player.verty2);
  // tri[3].position=sf::Vector2f(player.x+player.vertx2,player.y+player.verty2);
  // rt.draw(tri);

  rt.display();
  sf::Sprite temp(rt.getTexture());
  window.draw(temp);
  window.display();
}
int init() {
  if(!texturemap["Player"].loadFromFile("assets/images/Maphie.png")||
  !texturemap["Background1"].loadFromFile("assets/images/Background.png"))return -1;
  view.setCenter({float(player.x+64),-64});
  window.setFramerateLimit(60);
  window.setVerticalSyncEnabled(true);
  groundvector.resize(3);
  groundvector[0]=ground{-32,0,4096,1};

  groundvector[1]=ground{128,-48,32,1};
  groundvector[2]=ground{192,-48,32,1};
  return 0;
}
int main() {
  if(init()==-1)return 0;
  while (window.isOpen()) {
    windowset();
    input();
    update();
    render();
  }
  return 0;
}
