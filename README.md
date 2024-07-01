# ESP32-FileServer-ESP-IDF

![ESP32 File Server with SD card]
<p>
    <img src="ESP32-Server-SD.jpg" width="384"/>
</p>

<p>
Key wiring:</br>
    <ul>
        <li>D15 -> MOSI</li>
        <li>D4 -> MISO</li>
        <li>D13 -> CS</li>
        <li>D14 -> CLK</li>
    </ul>
</p>

```html
<!DOCTYPE html>
    <head><meta charset=\"utf-8\">
        <title>ESP32 Private Portable Files Server</title>
        <style>
    html {font-family: sans-serif;-ms-text-size-adjust: 100%;-webkit-text-size-adjust: 100%;}
    body {margin: 0;}
    article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary {display: block;}
    a {background-color: transparent;}
    a:active,a:hover {outline: 0;}
    b,strong {font-weight: bold;}
    h1 {font-size: 2em;margin: 0.67em 0;}
    mark {background: #ff0;color: #000;}
    small {font-size: 80%;}
    hr {-moz-box-sizing: content-box;box-sizing: content-box;height: 0;}
    button,input,optgroup,select,textarea {color: inherit;font: inherit;margin: 0;}
    button {overflow: visible;}
    button,select {text-transform: none;}
    button,html input[type=\"button\"],input[type=\"reset\"],input[type=\"submit\"] {-webkit-appearance: button;cursor: pointer;}
    button::-moz-focus-inner,input::-moz-focus-inner {border: 0;padding: 0;}
    input {line-height: normal;}
    textarea {overflow: auto;}
    table {border-collapse: collapse;border-spacing: 0;}
    td,th {padding: 0;}
    .container {position: relative;width: 100%;max-width: 960px;margin: 0 auto;padding: 0 20px;box-sizing: border-box;}
    .column,.columns {width: 100%;float: left;box-sizing: border-box;margin-bottom: 1.5rem; }
    @media (min-width: 400px) {.container {width: 85%;padding: 0; }}
    @media (min-width: 550px) {
    .container {width: 80%; }.column,.columns {margin-left: 4%;}.column:first-child,.columns:first-child {margin-left: 0;}
    .one.column,.one.columns{width:4.66666666667%;}.two.columns{width: 13.3333333333%;}.three.columns{width: 22%;}.four.columns{width: 30.6666666667%;}.five.columns{ width:39.3333333333%;}.six.columns{width: 48%;}.seven.columns{ width: 56.6666666667%;}.eight.columns{width: 65.3333333333%;}.nine.columns{ width:74.0%;}.ten.columns{width: 82.6666666667%;}.eleven.columns{ width:91.3333333333%;}.twelve.columns{width: 100%;margin-left: 0;}.one-third.column{ width:30.6666666667%;}.two-thirds.column{width:65.3333333333%;}.one-half.column{ width:48%;}
    .offset-by-one.column,.offset-by-one.columns{margin-left: 8.66666666667%;}.offset-by-two.column,.offset-by-two.columns{margin-left: 17.3333333333%; }.offset-by-three.column,.offset-by-three.columns{margin-left: 26%;}.offset-by-four.column,.offset-by-four.columns{margin-left: 34.6666666667%; }.offset-by-five.column,.offset-by-five.columns{margin-left: 43.3333333333%;}.offset-by-six.column,.offset-by-six.columns{margin-left: 52%;}.offset-by-seven.column,.offset-by-seven.columns{margin-left: 60.6666666667%;}.offset-by-eight.column,  .offset-by-eight.columns{margin-left: 69.3333333333%;}.offset-by-nine.column,.offset-by-nine.columns{ margin-left: 78.0%;}.offset-by-ten.column,.offset-by-ten.columns{margin-left: 86.6666666667%; }.offset-by-eleven.column,.offset-by-eleven.columns{margin-left: 95.3333333333%; }.offset-by-one-third.column,.offset-by-one-third.columns{margin-left: 34.6666666667%;}.offset-by-two-thirds.column,.offset-by-two-thirds.columns{margin-left: 69.3333333333%; }.offset-by-one-half.column,.offset-by-one-half.columns{margin-left: 52%;}
    }
    html {font-size: 62.5%; }
    body {font-size: 1.5em;line-height: 1.6;font-weight: 400;font-family: \"Raleway\", \"HelveticaNeue\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;color: #222; }
    h1, h2, h3, h4, h5, h6 {margin-top: 0;margin-bottom: 2rem;font-weight: 300; }
    h1 { font-size: 4.0rem; line-height: 1.2;  letter-spacing: -.1rem;}
    h2 { font-size: 3.6rem; line-height: 1.25; letter-spacing: -.1rem; }
    h3 { font-size: 3.0rem; line-height: 1.3;  letter-spacing: -.1rem; }
    h4 { font-size: 2.4rem; line-height: 1.35; letter-spacing: -.08rem; }
    h5 { font-size: 1.8rem; line-height: 1.5;  letter-spacing: -.05rem; }
    h6 { font-size: 1.5rem; line-height: 1.6;  letter-spacing: 0; }
    @media (min-width: 550px) {h1 { font-size: 5.0rem; }h2 { font-size: 4.2rem; }h3 { font-size: 3.6rem; }h4 { font-size: 3.0rem; }h5 { font-size: 2.4rem; }h6 { font-size: 1.5rem; }}
    p {margin-top: 0; }
    a {color: #1EAEDB; }
    a:hover {color: #0FA0CE; }
    .button,button,input[type=\"submit\"],input[type=\"reset\"],input[type=\"button\"] {display: inline-block;height: 38px;padding: 0 30px;color: #555;text-align: center;font-size: 11px;font-weight: 600;line-height: 38px;letter-spacing: .1rem;text-transform: uppercase;text-decoration: none;white-space: nowrap;background-color: transparent;border-radius: 4px;border: 1px solid #bbb;cursor: pointer;box-sizing: border-box; }
    .button:hover,button:hover,input[type=\"submit\"]:hover,input[type=\"reset\"]:hover,input[type=\"button\"]:hover,.button:focus,button:focus,input[type=\"submit\"]:focus,input[type=\"reset\"]:focus,input[type=\"button\"]:focus {color: #333;border-color: #888;outline: 0; }
    .button.button-primary,button.button-primary,input[type=\"submit\"].button-primary,input[type=\"reset\"].button-primary,input[type=\"button\"].button-primary {color: #FFF;background-color: #33C3F0;border-color: #33C3F0; }
    .button.button-delete,button.button-delete,input[type=\"submit\"].button-delete,input[type=\"reset\"].button-delete,input[type=\"button\"].button-delete {color: #333;background-color: #fff;border-color: #e01010; }
    .button.button-primary:hover,button.button-primary:hover,input[type=\"submit\"].button-primary:hover,input[type=\"reset\"].button-primary:hover,input[type=\"button\"].button-primary:hover,.button.button-primary:focus,button.button-primary:focus,input[type=\"submit\"].button-primary:focus,input[type=\"reset\"].button-primary:focus,input[type=\"button\"].button-primary:focus {color: #FFF;background-color: #1EAEDB;border-color: #1EAEDB; }
    .button.button-delete:hover,button.button-delete:hover,input[type=\"submit\"].button-delete:hover,input[type=\"reset\"].button-delete:hover,input[type=\"button\"].button-delete:hover,.button.button-delete:focus,button.button-delete:focus,input[type=\"submit\"].button-delete:focus,input[type=\"reset\"].button-delete:focus,input[type=\"button\"].button-delete:focus {color: #333;background-color: #e01010;border-color: #e01010; }
    input[type=\"email\"],input[type=\"number\"],input[type=\"search\"],input[type=\"text\"],input[type=\"tel\"],input[type=\"url\"],input[type=\"password\"],textarea,select {height: 38px;padding: 6px 10px;background-color: #fff;border: 1px solid #D1D1D1;border-radius: 4px;box-shadow: none;box-sizing: border-box;}input[type=\"email\"],input[type=\"number\"],input[type=\"search\"],input[type=\"text\"],input[type=\"tel\"],input[type=\"url\"],input[type=\"password\"],textarea {-webkit-appearance: none;-moz-appearance: none;appearance: none;}textarea {min-height: 65px;padding-top: 6px;padding-bottom: 6px; }input[type=\"email\"]:focus,input[type=\"number\"]:focus,input[type=\"search\"]:focus,input[type=\"text\"]:focus,input[type=\"tel\"]:focus,input[type=\"url\"]:focus,input[type=\"password\"]:focus,textarea:focus,select:focus {border: 1px solid #33C3F0;outline: 0; }label,legend {display: block;margin-bottom: .5rem;font-weight: 600; }fieldset {padding: 0;border-width: 0; }input[type=\"checkbox\"],input[type=\"radio\"] {display: inline; }label > .label-body {display: inline-block;margin-left: .5rem;font-weight: normal; }
    ul {list-style: circle inside; }ol {list-style: decimal inside; }ol, ul {padding-left: 0;margin-top: 0; }ul ul,ul ol,ol ol,ol ul {margin: 1.5rem 0 1.5rem 3rem;font-size: 90%; }li {margin-bottom: 1rem; }
    code {padding: .2rem .5rem;margin: 0 .2rem;font-size: 90%;white-space: nowrap;background: #F1F1F1;border: 1px solid #E1E1E1;border-radius: 4px; }pre > code {display: block;padding: 1rem 1.5rem;white-space: pre; }
    th,td {padding: 12px 15px;text-align: left;border-bottom: 1px solid #E1E1E1; }th:first-child,td:first-child {padding-left: 0; }th:last-child,td:last-child {padding-right: 0; }
    button,.button {margin-bottom: 1rem; }input,textarea,select,fieldset {margin-bottom: 1.5rem; }pre,blockquote,dl,figure,table,p,ul,ol,form {margin-bottom: 2.5rem; }
    .container:after,.row:after,.u-cf {content: "";display: table;clear: both; }
    </style>
    <link href=\"//fonts.googleapis.com/css?family=Raleway:400,300,600\" rel=\"stylesheet\" type=\"text/css\">
    </head>
    <html>
        <body>
            <div class=\"container\">
```
