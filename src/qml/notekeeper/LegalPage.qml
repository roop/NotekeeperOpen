import QtQuick 1.1
import QtWebKit 1.0
import "../native"

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Legal"
        iconPath: "images/titlebar/settings_white.svg"
    }

    Flickable {
        id: flickable
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        contentHeight: webView.height
        contentWidth: webView.width
        WebView {
            id: webView
            anchors { left: parent.left; top: parent.top; }
            preferredWidth: flickable.width
            preferredHeight: flickable.height
            settings.standardFontFamily: _UI.fontFamilyRegular
            property string bgColor: (_UI.isDarkColorScheme? "#000" : "#fff")
            property string textColor: (_UI.isDarkColorScheme? "#fff" : "#000")
            property string linkColor: _UI.colorLinkString

            html: "" +
                  "<style type=\"text/css\">\n" +
                  "body { background-color: " + bgColor + "; color: " + textColor + "; }\n" +
                  "a    { color: " + linkColor + "; }\n" +
                  "</style>\n" +
                  "<body>" +

                  "<h3>Notekeeper</h3>" +
                  "<pre>\n" + legalTexts.notekeeper + "</pre>\n" +
                  "<h3>Evernote Cloud API</h3>" +
                  "<pre>\n" + legalTexts.edam + "</pre>\n" +
                  "<h3>Apache Thrift</h3>\n" +
                  "<pre>\n" + legalTexts.thrift + "</pre>\n" +
                  "<h3>JS-humanize</h3>\n" +
                  "<pre>\n" + legalTexts.humanize + "</pre>\n" +
                  "<h3>Qt Components</h3>\n" +
                  "<pre>\n" + legalTexts.qt_components + "</pre>\n" +

                  "</body>";
        }
    }

    ScrollDecorator {
        flickableItem: flickable
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }

    QtObject {
        id: legalTexts
        property string notekeeper:
            " Copyright (c) 2011-2015 by Roopesh Chander <roop@roopc.net>.\n" +
            " All rights reserved.\n" +
            "\n" +
            " This product uses the Evernote Cloud API, but is not endorsed\n" +
            " or certified by Evernote.\n";
        property string edam:
            " Copyright (c) 2007-2012 by Evernote Corporation, All rights reserved.\n" +
            "\n" +
            " Use of the source code and binary libraries included in this package\n" +
            " is permitted under the following terms:\n" +
            "\n" +
            " Redistribution and use in source and binary forms, with or without\n" +
            " modification, are permitted provided that the following conditions\n" +
            " are met:\n" +
            "\n" +
            " 1. Redistributions of source code must retain the above copyright\n" +
            "    notice, this list of conditions and the following disclaimer.\n" +
            " 2. Redistributions in binary form must reproduce the above copyright\n" +
            "    notice, this list of conditions and the following disclaimer in the\n" +
            "    documentation and/or other materials provided with the distribution.\n" +
            "\n" +
            " THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\n" +
            " IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\n" +
            " OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n" +
            " IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\n" +
            " INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT\n" +
            " NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n" +
            " DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n" +
            " THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n" +
            " (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n" +
            " THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    \n" +
            "\n";
        property string thrift:
            "Copyright 2006-2010 The Apache Software Foundation.\n" +
            "\n" +
            "This product includes software developed at\n" +
            "The Apache Software Foundation (http://www.apache.org/).\n" +
            "\n" +
            "                                 Apache License\n" +
            "                           Version 2.0, January 2004\n" +
            "                        http://www.apache.org/licenses/\n" +
            "\n" +
            "   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION\n" +
            "\n" +
            "   1. Definitions.\n" +
            "\n" +
            "      \"License\" shall mean the terms and conditions for use, reproduction,\n" +
            "      and distribution as defined by Sections 1 through 9 of this document.\n" +
            "\n" +
            "      \"Licensor\" shall mean the copyright owner or entity authorized by\n" +
            "      the copyright owner that is granting the License.\n" +
            "\n" +
            "      \"Legal Entity\" shall mean the union of the acting entity and all\n" +
            "      other entities that control, are controlled by, or are under common\n" +
            "      control with that entity. For the purposes of this definition,\n" +
            "      \"control\" means (i) the power, direct or indirect, to cause the\n" +
            "      direction or management of such entity, whether by contract or\n" +
            "      otherwise, or (ii) ownership of fifty percent (50%) or more of the\n" +
            "      outstanding shares, or (iii) beneficial ownership of such entity.\n" +
            "\n" +
            "      \"You\" (or \"Your\") shall mean an individual or Legal Entity\n" +
            "      exercising permissions granted by this License.\n" +
            "\n" +
            "      \"Source\" form shall mean the preferred form for making modifications,\n" +
            "      including but not limited to software source code, documentation\n" +
            "      source, and configuration files.\n" +
            "\n" +
            "      \"Object\" form shall mean any form resulting from mechanical\n" +
            "      transformation or translation of a Source form, including but\n" +
            "      not limited to compiled object code, generated documentation,\n" +
            "      and conversions to other media types.\n" +
            "\n" +
            "      \"Work\" shall mean the work of authorship, whether in Source or\n" +
            "      Object form, made available under the License, as indicated by a\n" +
            "      copyright notice that is included in or attached to the work\n" +
            "      (an example is provided in the Appendix below).\n" +
            "\n" +
            "      \"Derivative Works\" shall mean any work, whether in Source or Object\n" +
            "      form, that is based on (or derived from) the Work and for which the\n" +
            "      editorial revisions, annotations, elaborations, or other modifications\n" +
            "      represent, as a whole, an original work of authorship. For the purposes\n" +
            "      of this License, Derivative Works shall not include works that remain\n" +
            "      separable from, or merely link (or bind by name) to the interfaces of,\n" +
            "      the Work and Derivative Works thereof.\n" +
            "\n" +
            "      \"Contribution\" shall mean any work of authorship, including\n" +
            "      the original version of the Work and any modifications or additions\n" +
            "      to that Work or Derivative Works thereof, that is intentionally\n" +
            "      submitted to Licensor for inclusion in the Work by the copyright owner\n" +
            "      or by an individual or Legal Entity authorized to submit on behalf of\n" +
            "      the copyright owner. For the purposes of this definition, \"submitted\"\n" +
            "      means any form of electronic, verbal, or written communication sent\n" +
            "      to the Licensor or its representatives, including but not limited to\n" +
            "      communication on electronic mailing lists, source code control systems,\n" +
            "      and issue tracking systems that are managed by, or on behalf of, the\n" +
            "      Licensor for the purpose of discussing and improving the Work, but\n" +
            "      excluding communication that is conspicuously marked or otherwise\n" +
            "      designated in writing by the copyright owner as \"Not a Contribution.\"\n" +
            "\n" +
            "      \"Contributor\" shall mean Licensor and any individual or Legal Entity\n" +
            "      on behalf of whom a Contribution has been received by Licensor and\n" +
            "      subsequently incorporated within the Work.\n" +
            "\n" +
            "   2. Grant of Copyright License. Subject to the terms and conditions of\n" +
            "      this License, each Contributor hereby grants to You a perpetual,\n" +
            "      worldwide, non-exclusive, no-charge, royalty-free, irrevocable\n" +
            "      copyright license to reproduce, prepare Derivative Works of,\n" +
            "      publicly display, publicly perform, sublicense, and distribute the\n" +
            "      Work and such Derivative Works in Source or Object form.\n" +
            "\n" +
            "   3. Grant of Patent License. Subject to the terms and conditions of\n" +
            "      this License, each Contributor hereby grants to You a perpetual,\n" +
            "      worldwide, non-exclusive, no-charge, royalty-free, irrevocable\n" +
            "      (except as stated in this section) patent license to make, have made,\n" +
            "      use, offer to sell, sell, import, and otherwise transfer the Work,\n" +
            "      where such license applies only to those patent claims licensable\n" +
            "      by such Contributor that are necessarily infringed by their\n" +
            "      Contribution(s) alone or by combination of their Contribution(s)\n" +
            "      with the Work to which such Contribution(s) was submitted. If You\n" +
            "      institute patent litigation against any entity (including a\n" +
            "      cross-claim or counterclaim in a lawsuit) alleging that the Work\n" +
            "      or a Contribution incorporated within the Work constitutes direct\n" +
            "      or contributory patent infringement, then any patent licenses\n" +
            "      granted to You under this License for that Work shall terminate\n" +
            "      as of the date such litigation is filed.\n" +
            "\n" +
            "   4. Redistribution. You may reproduce and distribute copies of the\n" +
            "      Work or Derivative Works thereof in any medium, with or without\n" +
            "      modifications, and in Source or Object form, provided that You\n" +
            "      meet the following conditions:\n" +
            "\n" +
            "      (a) You must give any other recipients of the Work or\n" +
            "          Derivative Works a copy of this License; and\n" +
            "\n" +
            "      (b) You must cause any modified files to carry prominent notices\n" +
            "          stating that You changed the files; and\n" +
            "\n" +
            "      (c) You must retain, in the Source form of any Derivative Works\n" +
            "          that You distribute, all copyright, patent, trademark, and\n" +
            "          attribution notices from the Source form of the Work,\n" +
            "          excluding those notices that do not pertain to any part of\n" +
            "          the Derivative Works; and\n" +
            "\n" +
            "      (d) If the Work includes a \"NOTICE\" text file as part of its\n" +
            "          distribution, then any Derivative Works that You distribute must\n" +
            "          include a readable copy of the attribution notices contained\n" +
            "          within such NOTICE file, excluding those notices that do not\n" +
            "          pertain to any part of the Derivative Works, in at least one\n" +
            "          of the following places: within a NOTICE text file distributed\n" +
            "          as part of the Derivative Works; within the Source form or\n" +
            "          documentation, if provided along with the Derivative Works; or,\n" +
            "          within a display generated by the Derivative Works, if and\n" +
            "          wherever such third-party notices normally appear. The contents\n" +
            "          of the NOTICE file are for informational purposes only and\n" +
            "          do not modify the License. You may add Your own attribution\n" +
            "          notices within Derivative Works that You distribute, alongside\n" +
            "          or as an addendum to the NOTICE text from the Work, provided\n" +
            "          that such additional attribution notices cannot be construed\n" +
            "          as modifying the License.\n" +
            "\n" +
            "      You may add Your own copyright statement to Your modifications and\n" +
            "      may provide additional or different license terms and conditions\n" +
            "      for use, reproduction, or distribution of Your modifications, or\n" +
            "      for any such Derivative Works as a whole, provided Your use,\n" +
            "      reproduction, and distribution of the Work otherwise complies with\n" +
            "      the conditions stated in this License.\n" +
            "\n" +
            "   5. Submission of Contributions. Unless You explicitly state otherwise,\n" +
            "      any Contribution intentionally submitted for inclusion in the Work\n" +
            "      by You to the Licensor shall be under the terms and conditions of\n" +
            "      this License, without any additional terms or conditions.\n" +
            "      Notwithstanding the above, nothing herein shall supersede or modify\n" +
            "      the terms of any separate license agreement you may have executed\n" +
            "      with Licensor regarding such Contributions.\n" +
            "\n" +
            "   6. Trademarks. This License does not grant permission to use the trade\n" +
            "      names, trademarks, service marks, or product names of the Licensor,\n" +
            "      except as required for reasonable and customary use in describing the\n" +
            "      origin of the Work and reproducing the content of the NOTICE file.\n" +
            "\n" +
            "   7. Disclaimer of Warranty. Unless required by applicable law or\n" +
            "      agreed to in writing, Licensor provides the Work (and each\n" +
            "      Contributor provides its Contributions) on an \"AS IS\" BASIS,\n" +
            "      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or\n" +
            "      implied, including, without limitation, any warranties or conditions\n" +
            "      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A\n" +
            "      PARTICULAR PURPOSE. You are solely responsible for determining the\n" +
            "      appropriateness of using or redistributing the Work and assume any\n" +
            "      risks associated with Your exercise of permissions under this License.\n" +
            "\n" +
            "   8. Limitation of Liability. In no event and under no legal theory,\n" +
            "      whether in tort (including negligence), contract, or otherwise,\n" +
            "      unless required by applicable law (such as deliberate and grossly\n" +
            "      negligent acts) or agreed to in writing, shall any Contributor be\n" +
            "      liable to You for damages, including any direct, indirect, special,\n" +
            "      incidental, or consequential damages of any character arising as a\n" +
            "      result of this License or out of the use or inability to use the\n" +
            "      Work (including but not limited to damages for loss of goodwill,\n" +
            "      work stoppage, computer failure or malfunction, or any and all\n" +
            "      other commercial damages or losses), even if such Contributor\n" +
            "      has been advised of the possibility of such damages.\n" +
            "\n" +
            "   9. Accepting Warranty or Additional Liability. While redistributing\n" +
            "      the Work or Derivative Works thereof, You may choose to offer,\n" +
            "      and charge a fee for, acceptance of support, warranty, indemnity,\n" +
            "      or other liability obligations and/or rights consistent with this\n" +
            "      License. However, in accepting such obligations, You may act only\n" +
            "      on Your own behalf and on Your sole responsibility, not on behalf\n" +
            "      of any other Contributor, and only if You agree to indemnify,\n" +
            "      defend, and hold each Contributor harmless for any liability\n" +
            "      incurred by, or claims asserted against, such Contributor by reason\n" +
            "      of your accepting any such warranty or additional liability.\n" +
            "\n" +
            "   END OF TERMS AND CONDITIONS\n" +
            "\n" +
            "   APPENDIX: How to apply the Apache License to your work.\n" +
            "\n" +
            "      To apply the Apache License to your work, attach the following\n" +
            "      boilerplate notice, with the fields enclosed by brackets \"[]\"\n" +
            "      replaced with your own identifying information. (Don't include\n" +
            "      the brackets!)  The text should be enclosed in the appropriate\n" +
            "      comment syntax for the file format. We also recommend that a\n" +
            "      file or class name and description of purpose be included on the\n" +
            "      same \"printed page\" as the copyright notice for easier\n" +
            "      identification within third-party archives.\n" +
            "\n" +
            "   Copyright [yyyy] [name of copyright owner]\n" +
            "\n" +
            "   Licensed under the Apache License, Version 2.0 (the \"License\");\n" +
            "   you may not use this file except in compliance with the License.\n" +
            "   You may obtain a copy of the License at\n" +
            "\n" +
            "       http://www.apache.org/licenses/LICENSE-2.0\n" +
            "\n" +
            "   Unless required by applicable law or agreed to in writing, software\n" +
            "   distributed under the License is distributed on an \"AS IS\" BASIS,\n" +
            "   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n" +
            "   See the License for the specific language governing permissions and\n" +
            "   limitations under the License.\n" +
            "\n" +
            "--------------------------------------------------\n" +
            "SOFTWARE DISTRIBUTED WITH THRIFT:\n" +
            "\n" +
            "The Apache Thrift software includes a number of subcomponents with\n" +
            "separate copyright notices and license terms. Your use of the source\n" +
            "code for the these subcomponents is subject to the terms and\n" +
            "conditions of the following licenses.\n" +
            "\n" +
            "--------------------------------------------------\n" +
            "Portions of the following files are licensed under the MIT License:\n" +
            "\n" +
            "  lib/erl/src/Makefile.am\n" +
            "\n" +
            "Please see doc/otp-base-license.txt for the full terms of this license.\n" +
            "\n" +
            "\n" +
            "--------------------------------------------------\n" +
            "The following files contain some portions of code contributed under\n" +
            "the Thrift Software License (see doc/old-thrift-license.txt), and relicensed\n" +
            "under the Apache 2.0 License:\n" +
            "\n" +
            "  compiler/cpp/Makefile.am\n" +
            "  compiler/cpp/src/generate/t_cocoa_generator.cc\n" +
            "  compiler/cpp/src/generate/t_cpp_generator.cc\n" +
            "  compiler/cpp/src/generate/t_csharp_generator.cc\n" +
            "  compiler/cpp/src/generate/t_erl_generator.cc\n" +
            "  compiler/cpp/src/generate/t_hs_generator.cc\n" +
            "  compiler/cpp/src/generate/t_java_generator.cc\n" +
            "  compiler/cpp/src/generate/t_ocaml_generator.cc\n" +
            "  compiler/cpp/src/generate/t_perl_generator.cc\n" +
            "  compiler/cpp/src/generate/t_php_generator.cc\n" +
            "  compiler/cpp/src/generate/t_py_generator.cc\n" +
            "  compiler/cpp/src/generate/t_rb_generator.cc\n" +
            "  compiler/cpp/src/generate/t_st_generator.cc\n" +
            "  compiler/cpp/src/generate/t_xsd_generator.cc\n" +
            "  compiler/cpp/src/main.cc\n" +
            "  compiler/cpp/src/parse/t_field.h\n" +
            "  compiler/cpp/src/parse/t_program.h\n" +
            "  compiler/cpp/src/platform.h\n" +
            "  compiler/cpp/src/thriftl.ll\n" +
            "  compiler/cpp/src/thrifty.yy\n" +
            "  lib/csharp/src/Protocol/TBinaryProtocol.cs\n" +
            "  lib/csharp/src/Protocol/TField.cs\n" +
            "  lib/csharp/src/Protocol/TList.cs\n" +
            "  lib/csharp/src/Protocol/TMap.cs\n" +
            "  lib/csharp/src/Protocol/TMessage.cs\n" +
            "  lib/csharp/src/Protocol/TMessageType.cs\n" +
            "  lib/csharp/src/Protocol/TProtocol.cs\n" +
            "  lib/csharp/src/Protocol/TProtocolException.cs\n" +
            "  lib/csharp/src/Protocol/TProtocolFactory.cs\n" +
            "  lib/csharp/src/Protocol/TProtocolUtil.cs\n" +
            "  lib/csharp/src/Protocol/TSet.cs\n" +
            "  lib/csharp/src/Protocol/TStruct.cs\n" +
            "  lib/csharp/src/Protocol/TType.cs\n" +
            "  lib/csharp/src/Server/TServer.cs\n" +
            "  lib/csharp/src/Server/TSimpleServer.cs\n" +
            "  lib/csharp/src/Server/TThreadPoolServer.cs\n" +
            "  lib/csharp/src/TApplicationException.cs\n" +
            "  lib/csharp/src/Thrift.csproj\n" +
            "  lib/csharp/src/Thrift.sln\n" +
            "  lib/csharp/src/TProcessor.cs\n" +
            "  lib/csharp/src/Transport/TServerSocket.cs\n" +
            "  lib/csharp/src/Transport/TServerTransport.cs\n" +
            "  lib/csharp/src/Transport/TSocket.cs\n" +
            "  lib/csharp/src/Transport/TStreamTransport.cs\n" +
            "  lib/csharp/src/Transport/TTransport.cs\n" +
            "  lib/csharp/src/Transport/TTransportException.cs\n" +
            "  lib/csharp/src/Transport/TTransportFactory.cs\n" +
            "  lib/csharp/ThriftMSBuildTask/Properties/AssemblyInfo.cs\n" +
            "  lib/csharp/ThriftMSBuildTask/ThriftBuild.cs\n" +
            "  lib/csharp/ThriftMSBuildTask/ThriftMSBuildTask.csproj\n" +
            "  lib/rb/lib/thrift.rb\n" +
            "  lib/st/README\n" +
            "  lib/st/thrift.st\n" +
            "  test/OptionalRequiredTest.cpp\n" +
            "  test/OptionalRequiredTest.thrift\n" +
            "  test/ThriftTest.thrift\n" +
            "\n" +
            "--------------------------------------------------\n" +
            "For the aclocal/ax_boost_base.m4 and contrib/fb303/aclocal/ax_boost_base.m4 components:\n" +
            "\n" +
            "#   Copyright (c) 2007 Thomas Porschberg <thomas@randspringer.de>\n" +
            "#\n" +
            "#   Copying and distribution of this file, with or without\n" +
            "#   modification, are permitted in any medium without royalty provided\n" +
            "#   the copyright notice and this notice are preserved.\n" +
            "\n" +
            "--------------------------------------------------\n" +
            "For the compiler/cpp/src/md5.[ch] components:\n" +
            "\n" +
            "/*\n" +
            "  Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.\n" +
            "\n" +
            "  This software is provided 'as-is', without any express or implied\n" +
            "  warranty.  In no event will the authors be held liable for any damages\n" +
            "  arising from the use of this software.\n" +
            "\n" +
            "  Permission is granted to anyone to use this software for any purpose,\n" +
            "  including commercial applications, and to alter it and redistribute it\n" +
            "  freely, subject to the following restrictions:\n" +
            "\n" +
            "  1. The origin of this software must not be misrepresented; you must not\n" +
            "     claim that you wrote the original software. If you use this software\n" +
            "     in a product, an acknowledgment in the product documentation would be\n" +
            "     appreciated but is not required.\n" +
            "  2. Altered source versions must be plainly marked as such, and must not be\n" +
            "     misrepresented as being the original software.\n" +
            "  3. This notice may not be removed or altered from any source distribution.\n" +
            "\n" +
            "  L. Peter Deutsch\n" +
            "  ghost@aladdin.com\n" +
            "\n" +
            " */\n" +
            "\n" +
            "---------------------------------------------------\n" +
            "For the lib/rb/setup.rb: Copyright (c) 2000-2005 Minero Aoki,\n" +
            "lib/ocaml/OCamlMakefile and lib/ocaml/README-OCamlMakefile components:\n" +
            "     Copyright (C) 1999 - 2007  Markus Mottl\n" +
            "\n" +
            "Licensed under the terms of the GNU Lesser General Public License 2.1\n" +
            "(see doc/lgpl-2.1.txt for the full terms of this license)\n";
        property string humanize:
            "Copyright (c) 2012 TitanFile Inc. https://www.titanfile.com/\n" +
            "\n" +
            "Permission is hereby granted, free of charge, to any person obtaining\n" +
            "a copy of this software and associated documentation files (the\n" +
            "\"Software\"), to deal in the Software without restriction, including\n" +
            "without limitation the rights to use, copy, modify, merge, publish,\n" +
            "distribute, sublicense, and/or sell copies of the Software, and to\n" +
            "permit persons to whom the Software is furnished to do so, subject to\n" +
            "the following conditions:\n" +
            "\n" +
            "The above copyright notice and this permission notice shall be\n" +
            "included in all copies or substantial portions of the Software.\n" +
            "\n" +
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n" +
            "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n" +
            "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n" +
            "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE\n" +
            "LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION\n" +
            "OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION\n" +
            "WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
        property string qt_components:
            "Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).\n" +
            "All rights reserved.\n" +
            "Contact: Nokia Corporation (qt-info@nokia.com)\n" +
            "\n" +
            "Qt Components project\n" +
            "\n" +
            "Redistribution and use in source and binary forms, with or without\n" +
            "modification, are permitted provided that the following conditions are\n" +
            "met:\n" +
            "  * Redistributions of source code must retain the above copyright\n" +
            "    notice, this list of conditions and the following disclaimer.\n" +
            "  * Redistributions in binary form must reproduce the above copyright\n" +
            "    notice, this list of conditions and the following disclaimer in\n" +
            "    the documentation and/or other materials provided with the\n" +
            "    distribution.\n" +
            "  * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor\n" +
            "    the names of its contributors may be used to endorse or promote\n" +
            "    products derived from this software without specific prior written\n" +
            "    permission.\n" +
            "\n" +
            "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n" +
            "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n" +
            "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n" +
            "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n" +
            "OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n" +
            "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n" +
            "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n" +
            "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n" +
            "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n" +
            "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n" +
            "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    }
}

