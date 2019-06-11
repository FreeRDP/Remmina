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
            pswdElement.value = 'PWDPLACEHOLDER';
            formNode = pswdElement.form;
            usrField = formNode.querySelectorAll("input[type='text']");
            usrField.forEach(function(usrElement) {
                if (usrElement.getAttribute('autocomplete') !== 'new-password') {
                    usrElement.value = 'USRPLACEHOLDER';
                }
                usrId = usrElement.id;
                usrElement.dispatchEvent(evt);
            });
            pswdElement.dispatchEvent(evt);
        });
    }
}

var frames = window.frames;
var i;

if (frames.length != 0) {
    for (i = 0; i < frames.length; i++) {
        setLoginFields(frames[i].id);
    }
} else {
    setLoginFields(null);
}
