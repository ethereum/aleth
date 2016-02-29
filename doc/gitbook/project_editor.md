# Project Editor

You can use projects to manage the creation and testing of a dapp. The project will contain data related to both backend and frontend as well as the data related to your scenarios (blockchain interaction) for debugging and testing. The related files will be created and saved automatically in the project directory.

## Creating a new project

The development of a dapp start with the creation of a new project.
Create a new project in the “edit” menu. Enter the project name, e.g. "Ratings" and select a path for the project file.

## Editing backend contract file 

By default, a new project contains a contract “Contract” for backend development on the blockchain using the Solidity language and the “index.html” for the frontend. Check the Solidity tutorial for references. 

Edit the empty default contract “Contract”, e.g. 
   
   ```
   contract Rating {
        
        function setRating(bytes32 _key, uint256 _value) {
        
            ratings[_key] = _value;
        
        }
        mapping (bytes32 => uint256) public ratings;
    }```

Check the Solidity tutorial for help getting started with the solidity programming language.

Save changes

### Editing frontend html files

Select default index.html file and enter the following code
  ```
   <!doctype>
    <html>
    <head>
    <script type="text/javascript">
    function getRating() {
        var param = document.getElementById("query").value;
        var res = contracts["Rating"].contract.ratings(param);
        document.getElementById("queryres").innerText = res;
    }

    function setRating() {
        var key = document.getElementById("key").value;
        var value = parseInt(document.getElementById("value").value);
        var res = contracts["Rating"].contract.setRating(key, value);
    }
    </script>
    </head>
    <body bgcolor="#E6E6FA">
        <h1>Ratings</h1>
        <div>
            Store:
            <input type="string" id="key">
            <input type="number" id="value">
            <button onclick="setRating()">Save</button>
        </div>
        <div>
            Query:
            <input type="string" id="query" onkeyup='getRating()'>
            <div id="queryres"></div>
        </div>
    </body>
    </html>
```

Then it is possible to add many contract files as well as many HTML, JavaScript, css files


