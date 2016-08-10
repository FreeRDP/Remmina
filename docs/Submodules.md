## How to add a submodule 

```
git submodule add https://github.com/FreeRDP/FreeRDP.git FreeRDP
cd FreeRDP
git checkout ed3d9526b2
cd ..   
git commit -am "Added submodule as ./FreeRDP"  
git push
```

## How to clone
```
git clone https://github.com/FreeRDP/Remmina.git Remmina
cd Remmina
git submodule init
git submodule update
```