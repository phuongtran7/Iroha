<h1 align="center">
  Iroha
  <br>
</h1>

<h4 align="center">A simple Trello client inside terminal</h4>

![screenshot](/Iroha_Sample.JPG)

### How To Build

1. Install all dependencies with Microsoft's [vcpkg](https://github.com/Microsoft/vcpkg).
    * `vcpkg install boost-beast`
    * `vcpkg install fmt`
    * `vcpkg install yaml-cpp`
    * `vcpkg install openssl`
    * `vcpkg install nlohmann_json`

2. Clone the project: `git clone https://github.com/phuongtran7/Iroha`.
3. Build:

```
cd Iroha
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake ..
cmake --build .
```

`Iroha` also uses [tabulate](https://github.com/p-ranav/tabulate) and [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing). However, due to the nature of these libraries, they are all kept inside the `include` folder.

### Usage

Iroha currently only support five main commands for now:

    View [ID]
    Create [ID]
    Update [ID]
    Quit
    Help

*ID*

The `ID` here is a simple local ID system to keep track of what should be displayed or updated. The syntax for an ID is: `[Board ID]-[List ID]-[Card ID]`.

*View*

The `view` command is used for displaying all the item in a particular place.

* `view`: Will display all the open `Boards` in Trello account.
* `view 0`: Will display all the `Lists` inside the 0 index `Board`.
* `view 0-1`: Will display the `Cards` inside 0 index `Board` and 1 index `List`.

*Create*

The `create` command is used for creating a new item in a particular place.

* `create`: Will create a new `Board` inside the Trello account.
* `create 1`: Will create a new `Lists` inside the 1 index `Board`.
* `create 1-2`: Will create a new `Cards` inside 1 index `Board` and 2 index `List`.

*Update*

The `update` command is used for updating an item in a particular place.

* `update 0`: Will update the 0 index `Board`.
* `update 0-1`: Will update the 1 index `List` in 0 index `Board`.
* `update 2-3-4`: Will update the 4 index `Card` in the 3 index `List` that is inside the 2 index `Board`.

*Quit*

Gracefully shutdown the application and connection to Trello.

*Help*

Display all the available commands.

### Trello Authorization

`Iroha` uses set of key-token to authorize action with Trello.

1. Get API Key and Token from [Trello](https://developer.atlassian.com/cloud/trello/guides/rest-api/api-introduction/).
2. Prepare a `Config.yaml` file with content:

```yaml
Key = "DEVELOPER KEY"
Token = "TOKEN"
```
and then put it next to the compiled executable.

### Limitation

Currently, `Iroha` doesn't have any caching functionality. So any action will result in a REST query to Trello.

Also, due to not having a cache system, every action has to be from top-down. So `Iroha` cannot execute `update 2-3-4` right after the application starts up. It has to first load all the available `Boards` then, load the `Lists` inside a `Board` and finally all the `Cards`.

However, after loading, any action can be execute freely.
