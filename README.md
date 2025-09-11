```
git clone --recurse-submodules https://github.com/apiel/zicFilterbank.git
```

might have to do this the first time:
```
cd libDaisy
make clean
make
cd ../DaisySP
make clean
make
```

build by running

```
make
```

upload with:

```
make program-dfu
```