T.assert(GG.rootPass() != null, "root pass is not null");

GPass pass0, pass1;

T.assert(pass0.next() == null, "default next is NULL");
pass0 --> pass1;
T.assert(pass0.next() == pass1, "next is pass1");
T.assert(pass1.next() == null, "pass1 next is NULL");
pass0 --< pass1;
T.assert(pass0.next() == null, "ungruck pass is null");


RenderPass rpass;
T.assert(rpass.target() == null, "default target is null");
Texture target;
rpass.target(target);
T.assert(rpass.target() == target, "target is target");
rpass.target(null);
T.assert(rpass.target() == null, "target is null again");