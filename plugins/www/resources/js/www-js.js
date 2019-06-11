/*jshint esversion: 6 */
function setLoginFields() {
  var evt = new Event('change');

  var frames = window.frames;

  var doc;
  var pswdField;
  var usrField;
  var formNode;

  if (frames.length != 0) {
    for (var i = 0; i < frames.length; i++) {
      doc = frames[i].document;
      pswdField = doc.querySelectorAll("input[type='password']");
      if ((pswdField !== undefined) && (pswdField !== null)) {
        break;
      }

    }
  } else {
    doc = window.document;
    pswdField = doc.querySelectorAll("input[type='password']");

  }


  if (pswdField !== undefined) {
    pswdField.forEach(function(pswdElement) {
      if (pswdElement.getAttribute('autocomplete') !== 'new-password') {
        pswdElement.value = 'PWDPLACEHOLDER';
      }

      formNode = pswdElement.form;
      usrField = formNode.querySelectorAll("input[type='text']");

      usrField.forEach(function(usrElement) {
        usrElement.value = 'USRPLACEHOLDER';
        usrElement.dispatchEvent(evt);
      });
      pswdElement.dispatchEvent(evt);
    });
  }
}


setLoginFields();
