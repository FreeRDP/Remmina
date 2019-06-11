/*jshint esversion: 6 */
function setLoginFields(frame) {
    var evt;
    evt = new Event('change');
    var pswdField;
    var usrField;
    var pwdId;
    var usrId;
    var formNode;
    if ((frame !== undefined) && (frame !== null)) {
        pswdField = document.getElementById(frame).contentDocument.querySelectorAll("input[type='password']");
    } else {
        pswdField = document.querySelectorAll("input[type='password']");

    }
    if (pswdField !== undefined) {
        pswdField.forEach(function(pswdElement) {
            if (pswdElement.getAttribute('autocomplete') !== 'new-password') {
                pswdElement.value = 'PWDPLACEHOLDER';
                var pswdElementId = pswdElement.id;
            }
            if ((frame !== undefined) && (frame !== null)) {
                // using reference to iframe (ifrm) obtained above
                var ifrm = document.getElementById(frame);
                //alert("Dealing with frame " + ifrm);
                var win = ifrm.contentWindow; // reference to iframe's window
                //alert("Windows is " + win);
                // reference to document in iframe
                var doc = ifrm.contentDocument? ifrm.contentDocument: ifrm.contentWindow.document;
                //alert("Document is " + doc);
                // reference to form named 'demoForm' in iframe
                var iframeElement = doc.getElementById(pswdElementId);
                //alert("iframeElement " + iframeElement);
                formNode = doc.getElementById(iframeElement.id);

                //formNode = document.getElementById(frame).contentWindow.getElementById(pswdElementId).form;
                alert("FormNode: " + formNode);
                usrField = formNode.querySelectorAll("input[type='text']");
                alert(usrField);
            } else {
                formNode = pswdElement.form;
                usrField = formNode.querySelectorAll("input[type='text']");
                alert(usrField);
            }
            alert("usrField:" + usrField);
            if ((doc !== undefined) && (doc !== null)) {
                usrField.forEach(function(usrElement) {

                    alert("iframe: " + doc.getElementById(usrElement.id));
                });
            } else {
                usrField.forEach(function(usrElement) {
                    usrElement.value = 'USRPLACEHOLDER';
                    //usrId = usrElement.id;
                    //usrElement.dispatchEvent(evt);
                });
            }
            pswdElement.dispatchEvent(evt);
        });
    }
}

var frames = window.frames;

if (frames.length != 0) {
    for (var i = 0; i < frames.length; i++) {
        setLoginFields(frames[i].frameElement.id);
    }
} else {
    setLoginFields(null);
}
