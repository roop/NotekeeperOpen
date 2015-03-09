/*
Copyright (c) 2012 TitanFile Inc. https://www.titanfile.com/

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
  Source: https://github.com/milanvrekic/JS-humanize/blob/master/humanize.js
*/

.pragma library

function number_format( number, decimals, dec_point, thousands_sep ) {
	// http://kevin.vanzonneveld.net
	// +   original by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
	// +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
	// +	 bugfix by: Michael White (http://crestidg.com)
	// +	 bugfix by: Benjamin Lupton
	// +	 bugfix by: Allan Jensen (http://www.winternet.no)
	// +	revised by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
	// *	 example 1: number_format(1234.5678, 2, '.', '');
	// *	 returns 1: 1234.57

	var n = number, c = isNaN(decimals = Math.abs(decimals)) ? 2 : decimals;
	var d = dec_point == undefined ? "," : dec_point;
	var t = thousands_sep == undefined ? "." : thousands_sep, s = n < 0 ? "-" : "";
	var i = parseInt(n = Math.abs(+n || 0).toFixed(c)) + "", j = (j = i.length) > 3 ? j % 3 : 0;

        var ret = s + (j ? i.substr(0, j) + t : "") + i.substr(j).replace(/(\d{3})(?=\d)/g, "$1" + t) + (c ? d + Math.abs(n - i).toFixed(c).slice(2) : "");
        if (c && (ret.length - d.length - 2 >= 0) && (ret.indexOf(d + "00") == (ret.length - d.length - 2))) {
            ret = ret.substr(0, ret.length - d.length - 2);
        }
        return ret;
}

function filesizeformat(filesize, dec_point_char) {
	if (filesize >= 1073741824) {
		 filesize = number_format(filesize / 1073741824, 2, dec_point_char, '') + ' GB';
	} else { 
		if (filesize >= 1048576) {
			filesize = number_format(filesize / 1048576, 2, dec_point_char, '') + ' MB';
   	} else { 
			if (filesize >= 1024) {
			filesize = number_format(filesize / 1024, 0) + ' KB';
  		} else {
			filesize = number_format(filesize, 0) + ' bytes';
			};
 		};
	};

	return filesize;
};

