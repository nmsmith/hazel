exception InvariantViolated;
module PP = Pretty.PP;
open SemanticsCore;
let (^^) = PP.(^^);
let taggedText = (cls, s) => PP.text(cls, s);
let dollar = taggedText("dollar", "$");
let kw = taggedText("kw");
let lparen = taggedText("lparen");
let rparen = taggedText("rparen");
let op = taggedText("op");
let var = s => taggedText("var", s);
let paletteName = s => taggedText("paletteName", s);
let space = taggedText("space", " ");
let rec id_of_rev_path = (prefix, rev_path) =>
  switch (rev_path) {
  | [] => prefix ++ "path_"
  | [x, ...xs] => id_of_rev_path(prefix, xs) ++ "_" ++ string_of_int(x)
  };
let cls_from_classes = (err_status, classes) =>
  switch (err_status) {
  | InHole(u) => [
      "in_err_hole",
      "in_err_hole_" ++ string_of_int(u),
      ...classes,
    ]
  | NotInHole => classes
  };
let cls_from = (err_status, cls) =>
  switch (err_status) {
  | InHole(u) => ["in_err_hole", "in_err_hole_" ++ string_of_int(u), cls]
  | NotInHole => [cls]
  };
let term_classes = (prefix, err_status, rev_path, classes, doc) => {
  let id' = id_of_rev_path(prefix, rev_path);
  PP.tagged(
    cls_from_classes(err_status, classes),
    Some((id', rev_path)),
    None,
    doc,
  );
};
let term = (prefix, err_status, rev_path, cls, doc) => {
  let id' = id_of_rev_path(prefix, rev_path);
  PP.tagged(cls_from(err_status, cls), Some((id', rev_path)), None, doc);
};
let term_with_attrs = (prefix, err_status, rev_path, classes, attrs, doc) => {
  let id' = id_of_rev_path(prefix, rev_path);
  PP.tagged(
    cls_from_classes(err_status, classes),
    Some((id', rev_path)),
    Some(attrs),
    doc,
  );
};

let of_var_binding = (prefix, rev_path, x) => {
  let id = id_of_rev_path(prefix, rev_path);
  PP.tagged(
    ["var_binding"],
    Some((id, rev_path)),
    None,
    taggedText("var", x),
  );
};

let optionalBreakSp = PP.optionalBreak(" ");
let optionalBreakNSp = PP.optionalBreak("");
let of_Parenthesized = (is_block, prefix, rev_path, r1) =>
  term(
    prefix,
    NotInHole,
    rev_path,
    "Parenthesized",
    is_block ?
      PP.blockBoundary
      ^^ lparen("(")
      ^^ PP.nestAbsolute(2, r1)
      ^^ PP.mandatoryBreak
      ^^ rparen(")") :
      lparen("(") ^^ r1 ^^ rparen(")"),
  );

let str_of_expr_op = op =>
  switch (op) {
  | UHExp.Plus => ("+", "op-Plus")
  | UHExp.Times => ("*", "op-Times")
  | UHExp.Space => (" ", "op-Space")
  };
let str_of_ty_op = op =>
  switch (op) {
  | UHTyp.Sum => ("|", "op-Sum")
  | UHTyp.Arrow => (LangUtil.typeArrowSym, "op-Arrow")
  };
let of_str_op_with_space = (op_s, op_cls) =>
  PP.tagged(
    ["seq-op", op_cls],
    None,
    None,
    taggedText("op-before-1", "​​")
    ^^ taggedText("op-before-2", "‌")
    ^^ taggedText("op-center", " " ++ op_s ++ " ")
    ^^ taggedText("op-after-1", "​")
    ^^ taggedText("op-after-2", "​"),
  );
let of_str_op_no_space = (op_s, op_cls) =>
  PP.tagged(
    ["seq-op", op_cls],
    None,
    None,
    taggedText("op-no-margin", op_s),
  );
let of_op_with_space = (str_of_op, op) => {
  let (op_s, op_cls) = str_of_op(op);
  of_str_op_with_space(op_s, op_cls);
};

let of_expr_op_with_space = of_op_with_space(str_of_expr_op);
let of_ty_op_with_space = of_op_with_space(str_of_ty_op);
let of_op_no_space = (str_of_op, op) => {
  let (op_s, op_cls) = str_of_op(op);
  of_str_op_no_space(op_s, op_cls);
};
let of_expr_op_no_space = of_op_no_space(str_of_expr_op);
let of_expr_op = op =>
  switch (op) {
  | UHExp.Plus => of_expr_op_with_space(op)
  | UHExp.Times => of_expr_op_no_space(op)
  | UHExp.Space => of_expr_op_no_space(op)
  };
let of_Hole = (prefix, err_status, rev_path, cls, hole_name) =>
  term(
    prefix,
    err_status,
    rev_path,
    cls,
    taggedText("hole-before-1", "​​")
    ^^ taggedText("hole-before-2", "​")
    ^^ taggedText("holeName", hole_name)
    ^^ taggedText("hole-after-1", "​")
    ^^ taggedText("hole-after-2", "​"),
  );

let precedence_const = 0;
let precedence_Sum = 1;
let precedence_Arrow = 2;
let precedence_ty = ty =>
  switch (ty) {
  | HTyp.Num => precedence_const
  | HTyp.Hole => precedence_const
  | HTyp.Sum(_, _) => precedence_Sum
  | HTyp.Arrow(_, _) => precedence_Arrow
  };
let of_Num = (prefix, rev_path) =>
  term(prefix, NotInHole, rev_path, "Num", kw("num"));
let of_ty_op = (cls, op_s, prefix, err_status, rev_path, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    cls,
    r1 ^^ optionalBreakSp ^^ op(op_s) ^^ optionalBreakSp ^^ r2,
  );

let of_Arrow = of_ty_op("Arrow", LangUtil.typeArrowSym);
let of_Sum = of_ty_op("Sum", "|");
let rec of_htype = (parenthesize, prefix, rev_path, ty) => {
  let d =
    switch (ty) {
    | HTyp.Num => of_Num(prefix, rev_path)
    | HTyp.Arrow(ty1, ty2) =>
      let rev_path1 = [0, ...rev_path];
      let rev_path2 = [1, ...rev_path];
      let paren1 = precedence_ty(ty1) >= precedence_Arrow;
      let paren2 = precedence_ty(ty2) > precedence_Arrow;
      let r1 = of_htype(paren1, prefix, rev_path1, ty1);
      let r2 = of_htype(paren2, prefix, rev_path2, ty2);
      of_Arrow(prefix, NotInHole, rev_path, r1, r2);
    | HTyp.Sum(ty1, ty2) =>
      let rev_path1 = [0, ...rev_path];
      let rev_path2 = [1, ...rev_path];
      let paren1 = precedence_ty(ty1) >= precedence_Sum;
      let paren2 = precedence_ty(ty2) > precedence_Sum;
      let r1 = of_htype(paren1, prefix, rev_path1, ty1);
      let r2 = of_htype(paren2, prefix, rev_path2, ty2);
      of_Sum(prefix, NotInHole, rev_path, r1, r2);
    | HTyp.Hole => of_Hole(prefix, NotInHole, rev_path, "Hole", "?")
    };
  parenthesize ? lparen("(") ^^ d ^^ rparen(")") : d;
};
let rec of_uhtyp = (prefix, rev_path, uty) =>
  switch (uty) {
  | UHTyp.Parenthesized(uty1) =>
    let rev_path1 = [0, ...rev_path];
    let r1 = of_uhtyp(prefix, rev_path1, uty1);
    of_Parenthesized(false, prefix, rev_path, r1);
  | UHTyp.Num => of_Num(prefix, rev_path)
  | UHTyp.OpSeq(skel, seq) =>
    term(
      prefix,
      NotInHole,
      rev_path,
      "OpSeq",
      of_uhtyp_skel(prefix, rev_path, skel, seq),
    )
  | UHTyp.Hole => of_Hole(prefix, NotInHole, rev_path, "Hole", "?")
  }
and of_uhtyp_skel = (prefix, rev_path, skel, seq) =>
  switch (skel) {
  | Skel.Placeholder(n) =>
    switch (OperatorSeq.seq_nth(n, seq)) {
    | Some(utyn) =>
      let rev_path_n = [n, ...rev_path];
      of_uhtyp(prefix, rev_path_n, utyn);
    | None => raise(InvariantViolated)
    }
  | Skel.BinOp(_, op, skel1, skel2) =>
    let r1 = of_uhtyp_skel(prefix, rev_path, skel1, seq);
    let r2 = of_uhtyp_skel(prefix, rev_path, skel2, seq);
    let op_pp = of_ty_op_with_space(op);
    PP.tagged(["skel-binop"], None, None, r1 ^^ op_pp ^^ r2);
  };

let of_Asc = (prefix, err_status, rev_path, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Asc",
    r1 ^^ space ^^ op(":") ^^ space ^^ r2,
  );

let classes_of_var_err_status = var_err_status =>
  switch (var_err_status) {
  | InVHole(u) => ["InVHole", "InVHole_" ++ string_of_int(u)]
  | NotInVHole => []
  };

let of_Var = (prefix, err_status, var_err_status, rev_path, x) =>
  term_classes(
    prefix,
    err_status,
    rev_path,
    ["Var", ...classes_of_var_err_status(var_err_status)],
    var(x),
  );

let of_Let = (prefix, err_status, rev_path, rx, rann, r1, r2) => {
  let first_part = PP.blockBoundary ^^ kw("let") ^^ space ^^ rx;
  let second_part =
    of_str_op_with_space("=", "let-equals")
    ^^ PP.nestAbsolute(2, r1)
    ^^ PP.mandatoryBreak
    ^^ r2;
  let view =
    switch (rann) {
    | Some(r) =>
      first_part ^^ of_str_op_with_space(":", "ann") ^^ r ^^ second_part
    | None => first_part ^^ second_part
    };
  term(prefix, err_status, rev_path, "Let", view);
};

let of_FixF = (prefix, err_status, rev_path, rx, rty, r1) => {
  let view =
    kw("fix")
    ^^ space
    ^^ rx
    ^^ of_str_op_no_space(":", "ann")
    ^^ rty
    ^^ taggedText("lambda-dot", ".")
    ^^ r1;
  term(prefix, err_status, rev_path, "Lam", view);
};

let of_Lam = (prefix, err_status, rev_path, rx, rann, r1) => {
  let first_part = taggedText("lambda-sym", LangUtil.lamSym) ^^ rx;
  let second_part = taggedText("lambda-dot", ".") ^^ r1;
  let view =
    switch (rann) {
    | Some(r) =>
      first_part ^^ of_str_op_no_space(":", "ann") ^^ r ^^ second_part
    | None => first_part ^^ second_part
    };
  term(prefix, err_status, rev_path, "Lam", view);
};

let of_Ap = (prefix, err_status, rev_path, r1, r2) =>
  term(prefix, err_status, rev_path, "Ap", r1 ^^ space ^^ r2);
let of_NumLit = (prefix, err_status, rev_path, n) =>
  term(
    prefix,
    err_status,
    rev_path,
    "NumLit",
    taggedText("number", string_of_int(n)),
  );

let of_Plus = (prefix, err_status, rev_path, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Plus",
    r1 ^^ optionalBreakNSp ^^ op(" +") ^^ optionalBreakSp ^^ r2,
  );

let of_Times = (prefix, err_status, rev_path, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Times",
    r1 ^^ op("*") ^^ optionalBreakNSp ^^ r2,
  );

let of_Space = (prefix, err_status, rev_path, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Space",
    r1 ^^ op(" ") ^^ optionalBreakNSp ^^ r2,
  );

let of_Inj = (prefix, err_status, rev_path, side, r) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Inj",
    kw("inj")
    ^^ lparen("[")
    ^^ kw(LangUtil.string_of_side(side))
    ^^ rparen("]")
    ^^ lparen("(")
    ^^ r
    ^^ rparen(")"),
  );

let of_InjAnn = (prefix, err_status, rev_path, rty, side, r) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Inj",
    kw("inj")
    ^^ lparen("[")
    ^^ kw(LangUtil.string_of_side(side))
    ^^ kw(",")
    ^^ optionalBreakSp
    ^^ rty
    ^^ rparen("]")
    ^^ lparen("(")
    ^^ PP.optionalBreak("")
    ^^ r
    ^^ rparen(")"),
  );

let of_Case = (prefix, err_status, rev_path, r1, rx, r2, ry, r3) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Case",
    PP.blockBoundary
    ^^ kw("case")
    ^^ space
    ^^ r1
    ^^ PP.mandatoryBreak
    ^^ kw("L")
    ^^ lparen("(")
    ^^ rx
    ^^ rparen(")")
    ^^ of_str_op_with_space(LangUtil.caseArrowSym, "case-arrow")
    ^^ PP.nestAbsolute(2, r2)
    ^^ PP.mandatoryBreak
    ^^ kw("R")
    ^^ lparen("(")
    ^^ ry
    ^^ rparen(")")
    ^^ of_str_op_with_space(LangUtil.caseArrowSym, "case-arrow")
    ^^ PP.nestAbsolute(2, r3),
  );

let of_CaseAnn = (prefix, err_status, rev_path, r1, rx, r2, ry, r3) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Case",
    PP.blockBoundary
    ^^ kw("case")
    ^^ space
    ^^ r1
    ^^ PP.mandatoryBreak
    ^^ kw("L")
    ^^ lparen("(")
    ^^ rx
    ^^ rparen(")")
    ^^ of_str_op_with_space(LangUtil.caseArrowSym, "case-arrow")
    ^^ PP.nestAbsolute(2, r2)
    ^^ PP.mandatoryBreak
    ^^ kw("R")
    ^^ lparen("(")
    ^^ ry
    ^^ rparen(")")
    ^^ of_str_op_with_space(LangUtil.caseArrowSym, "case-arrow")
    ^^ PP.nestAbsolute(2, r3),
  );

let cast_arrow = op(" ⇨ ");
let of_Cast = (prefix, err_status, rev_path, r1, rty1, rty2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Cast",
    r1 ^^ lparen("⟨") ^^ rty1 ^^ cast_arrow ^^ rty2 ^^ rparen("⟩"),
  );

let of_chained_Cast = (prefix, err_status, rev_path, r1, rty1, rty2, rty4) =>
  term(
    prefix,
    err_status,
    rev_path,
    "Cast",
    r1
    ^^ lparen("<")
    ^^ rty1
    ^^ cast_arrow
    ^^ rty2
    ^^ cast_arrow
    ^^ rty4
    ^^ rparen(">"),
  );

let failed_cast_arrow = taggedText("failed-cast-arrow", " ⇨ ");
let of_FailedCast = (prefix, err_status, rev_path, r1, rty1, rty2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "FailedCast",
    r1 ^^ lparen("<") ^^ rty1 ^^ failed_cast_arrow ^^ rty2 ^^ rparen(">"),
  );

let of_chained_FailedCast =
    (prefix, err_status, rev_path, r1, rty1, rty2, rty4) =>
  term(
    prefix,
    err_status,
    rev_path,
    "FailedCast",
    r1
    ^^ lparen("<")
    ^^ rty1
    ^^ failed_cast_arrow
    ^^ rty2
    ^^ failed_cast_arrow
    ^^ rty4
    ^^ rparen(">"),
  );

let is_block = e =>
  switch (e) {
  | UHExp.Tm(_, UHExp.Let(_, _, _, _)) => true
  | UHExp.Tm(_, UHExp.Case(_, _, _)) => true
  | _ => false
  };

type palette_stuff = {
  palette_view_ctx: Palettes.PaletteViewCtx.t,
  mk_editor_box: EditorBoxTypes.mk_editor_box,
  do_action: Action.t => unit,
};
let rec of_hexp = (palette_stuff, prefix, rev_path, e) =>
  switch (e) {
  | UHExp.Parenthesized(e1) =>
    let rev_path1 = [0, ...rev_path];
    let r1 = of_hexp(palette_stuff, prefix, rev_path1, e1);
    of_Parenthesized(is_block(e1), prefix, rev_path, r1);
  | UHExp.Tm(err_status, e') =>
    switch (e') {
    | UHExp.Asc(e1, ty) =>
      let rev_path1 = [0, ...rev_path];
      let rev_path2 = [1, ...rev_path];
      let r1 = of_hexp(palette_stuff, prefix, rev_path1, e1);
      let r2 = of_uhtyp(prefix, rev_path2, ty);
      of_Asc(prefix, err_status, rev_path, r1, r2);
    | UHExp.Var(var_err_status, x) =>
      of_Var(prefix, err_status, var_err_status, rev_path, x)
    | UHExp.Let(x, ann, e1, e2) =>
      let rx = of_var_binding(prefix, [0, ...rev_path], x);
      let rann =
        switch (ann) {
        | Some(uty1) => Some(of_uhtyp(prefix, [1, ...rev_path], uty1))
        | None => None
        };
      let r1 = of_hexp(palette_stuff, prefix, [2, ...rev_path], e1);
      let r2 = of_hexp(palette_stuff, prefix, [3, ...rev_path], e2);
      of_Let(prefix, err_status, rev_path, rx, rann, r1, r2);
    | UHExp.Lam(x, ann, e1) =>
      let rx = of_var_binding(prefix, [0, ...rev_path], x);
      let rann =
        switch (ann) {
        | Some(uty1) => Some(of_uhtyp(prefix, [1, ...rev_path], uty1))
        | None => None
        };
      let r1 = of_hexp(palette_stuff, prefix, [2, ...rev_path], e1);
      of_Lam(prefix, err_status, rev_path, rx, rann, r1);
    | UHExp.NumLit(n) => of_NumLit(prefix, err_status, rev_path, n)
    | UHExp.Inj(side, e) =>
      let rev_path1 = [0, ...rev_path];
      let r1 = of_hexp(palette_stuff, prefix, rev_path1, e);
      of_Inj(prefix, err_status, rev_path, side, r1);
    | UHExp.Case(e1, (x, e2), (y, e3)) =>
      let rev_path1 = [0, ...rev_path];
      let rev_pathx = [1, ...rev_path];
      let rev_path2 = [2, ...rev_path];
      let rev_pathy = [3, ...rev_path];
      let rev_path3 = [4, ...rev_path];
      let r1 = of_hexp(palette_stuff, prefix, rev_path1, e1);
      let rx = of_var_binding(prefix, rev_pathx, x);
      let r2 = of_hexp(palette_stuff, prefix, rev_path2, e2);
      let ry = of_var_binding(prefix, rev_pathy, y);
      let r3 = of_hexp(palette_stuff, prefix, rev_path3, e3);
      of_Case(prefix, err_status, rev_path, r1, rx, r2, ry, r3);
    | UHExp.EmptyHole(u) =>
      of_Hole(
        prefix,
        err_status,
        rev_path,
        "EmptyHole",
        string_of_int(u + 1),
      )
    | UHExp.OpSeq(skel, seq) =>
      term(
        prefix,
        err_status,
        rev_path,
        "OpSeq",
        of_skel(palette_stuff, prefix, rev_path, skel, seq),
      )
    | UHExp.ApPalette(name, serialized_model, (_, hole_map)) =>
      switch (
        Palettes.PaletteViewCtx.lookup(palette_stuff.palette_view_ctx, name)
      ) {
      | Some(serialized_view_fn) =>
        let updater = serialized_model => {
          palette_stuff.do_action(
            Action.MoveTo((List.rev(rev_path), Before)),
          );
          palette_stuff.do_action(
            Action.UpdateApPalette(UHExp.HoleRefs.ret(serialized_model)),
          );
        };
        let view = serialized_view_fn(serialized_model, updater);
        let paletteName =
          term(prefix, err_status, rev_path, "ApPalette", paletteName(name));
        let paletteDelim =
          term(prefix, err_status, rev_path, "ApPalette", dollar);
        let palettePrefix =
          switch (view) {
          | Inline(_) => paletteName
          | MultiLine(_) => paletteName ^^ PP.mandatoryBreak
          };
        let paletteSuffix =
          switch (view) {
          | Inline(_) => paletteDelim
          | MultiLine(_) => paletteDelim ^^ PP.mandatoryBreak
          };
        palettePrefix
        ^^ PP.paletteView(
             rev_path,
             view,
             hole_map,
             palette_stuff.mk_editor_box,
           )
        ^^ paletteSuffix;
      | None => raise(InvariantViolated)
      }
    }
  }
and of_skel = (palette_stuff, prefix, rev_path, skel, seq) =>
  switch (skel) {
  | Skel.Placeholder(n) =>
    switch (OperatorSeq.seq_nth(n, seq)) {
    | Some(en) =>
      let rev_path_n = [n, ...rev_path];
      of_hexp(palette_stuff, prefix, rev_path_n, en);
    | None => raise(InvariantViolated)
    }
  | Skel.BinOp(err_status, op, skel1, skel2) =>
    let r1 = of_skel(palette_stuff, prefix, rev_path, skel1, seq);
    let r2 = of_skel(palette_stuff, prefix, rev_path, skel2, seq);
    let op_pp =
      switch (op) {
      | UHExp.Times =>
        switch (Skel.rightmost_op(skel1)) {
        | Some(UHExp.Space) => of_expr_op_with_space(op)
        | _ =>
          switch (Skel.leftmost_op(skel2)) {
          | Some(UHExp.Space) => of_expr_op_with_space(op)
          | _ => of_expr_op(op)
          }
        }
      | _ => of_expr_op(op)
      };
    let cls = "skel-binop";
    let cls' = cls_from(err_status, cls);
    PP.tagged(cls', None, None, r1 ^^ op_pp ^^ r2);
  };

open Dynamics;
let of_bin_num_op = op =>
  switch (op) {
  | DHExp.Plus => taggedText("bin_num_op", " + ")
  | DHExp.Times => taggedText("bin_num_op", "*")
  };
let of_BinNumOp = (prefix, err_status, rev_path, op, r1, r2) =>
  term(
    prefix,
    err_status,
    rev_path,
    "BinNumOp",
    r1 ^^ of_bin_num_op(op) ^^ r2,
  );

let precedence_Ap = 1;
let precedence_Times = 2;
let precedence_Plus = 3;
let precedence_max = 4;
let rec precedence_dhexp = d =>
  DHExp.(
    switch (d) {
    | BoundVar(_)
    | FreeVar(_, _, _, _)
    | NumLit(_)
    | Inj(_, _, _)
    | EmptyHole(_, _, _)
    | Cast(_, _, _)
    | FailedCast(_, _, _) => precedence_const
    | Let(_, _, _)
    | FixF(_, _, _)
    | Lam(_, _, _)
    | Case(_, _, _) => precedence_max
    | Ap(_, _) => precedence_Ap
    | BinNumOp(Times, _, _) => precedence_Times
    | BinNumOp(Plus, _, _) => precedence_Plus
    | NonEmptyHole(_, _, _, d1) => precedence_dhexp(d1)
    }
  );

let hole_label_s = ((u, i)) =>
  string_of_int(u + 1) ++ ":" ++ string_of_int(i + 1);
let hole_label_of = inst => taggedText("holeName", hole_label_s(inst));
let cls_of_inst = ((u, i)) =>
  "hole-instance-" ++ string_of_int(u) ++ "-" ++ string_of_int(i);
let dbg_SHOW_SIGMAS = false;
let rec of_dhexp' =
        (instance_click_fn, parenthesize, prefix, err_status, rev_path, d) => {
  let doc =
    DHExp.(
      switch (d) {
      | BoundVar(x) => of_Var(prefix, err_status, NotInVHole, rev_path, x)
      | FreeVar(u, _, _, x) =>
        of_Var(prefix, err_status, InVHole(u), rev_path, x)
      | Let(x, d1, d2) =>
        let rev_pathx = [0, ...rev_path];
        let rev_path1 = [1, ...rev_path];
        let rev_path2 = [2, ...rev_path];
        let rx = of_var_binding(prefix, rev_pathx, x);
        let r1 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );
        let r2 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            rev_path2,
            d2,
          );
        of_Let(prefix, err_status, rev_path, rx, None, r1, r2);
      | FixF(x, ty, d1) =>
        let rx = of_var_binding(prefix, [0, ...rev_path], x);
        let rty = of_htype(false, prefix, [1, ...rev_path], ty);
        let r1 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            [2, ...rev_path],
            d1,
          );
        of_FixF(prefix, err_status, rev_path, rx, rty, r1);
      | Lam(x, ann, d1) =>
        let rx = of_var_binding(prefix, [0, ...rev_path], x);
        let rann = Some(of_htype(false, prefix, [1, ...rev_path], ann));
        let r1 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            [2, ...rev_path],
            d1,
          );
        of_Lam(prefix, err_status, rev_path, rx, rann, r1);
      | Ap(d1, d2) =>
        let rev_path1 = [0, ...rev_path];
        let rev_path2 = [1, ...rev_path];
        let paren1 = precedence_dhexp(d1) > precedence_Ap;
        let paren2 = precedence_dhexp(d2) >= precedence_Ap;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );

        let r2 =
          of_dhexp'(
            instance_click_fn,
            paren2,
            prefix,
            NotInHole,
            rev_path2,
            d2,
          );

        of_Ap(prefix, err_status, rev_path, r1, r2);
      | NumLit(n) => of_NumLit(prefix, err_status, rev_path, n)
      | BinNumOp(op, d1, d2) =>
        let rev_path1 = [0, ...rev_path];
        let rev_path2 = [1, ...rev_path];
        let prec_d = precedence_dhexp(d);
        let paren1 = precedence_dhexp(d1) > prec_d;
        let paren2 = precedence_dhexp(d2) >= prec_d;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );

        let r2 =
          of_dhexp'(
            instance_click_fn,
            paren2,
            prefix,
            NotInHole,
            rev_path2,
            d2,
          );

        of_BinNumOp(prefix, err_status, rev_path, op, r1, r2);
      | Inj(ty, side, d1) =>
        let rev_path1 = [0, ...rev_path];
        let rev_path2 = [1, ...rev_path];
        let r1 = of_htype(false, prefix, rev_path1, ty);
        let r2 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            rev_path2,
            d1,
          );

        of_InjAnn(prefix, err_status, rev_path, r1, side, r2);
      | Case(d1, (x, _), (y, _)) =>
        /* | Case(d1, (x, d2), (y, d3)) => */
        let rev_path1 = [0, ...rev_path];
        let rev_pathx = [1, ...rev_path];
        let rev_pathy = [2, ...rev_path];
        /* let rev_path2 = [1, ...rev_path];
           let rev_path3 = [2, ...rev_path]; */
        let r1 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );
        let rx = of_var_binding(prefix, rev_pathx, x);
        let ry = of_var_binding(prefix, rev_pathy, y);
        /* let r2 =
             of_dhexp'(
               instance_click_fn,
               false,
               prefix,
               NotInHole,
               rev_path2,
               d2,
             );

           let r3 =
             of_dhexp'(
               instance_click_fn,
               false,
               prefix,
               NotInHole,
               rev_path3,
               d3,
             ); */

        let elided = taggedText("elided", "...");

        of_CaseAnn(prefix, err_status, rev_path, r1, rx, elided, ry, elided);
      | EmptyHole(u, i, sigma) =>
        let inst = (u, i);
        let hole_label = hole_label_of(inst);
        let r =
          dbg_SHOW_SIGMAS ?
            hole_label ^^ of_sigma(instance_click_fn, prefix, rev_path, sigma) :
            hole_label;
        let attrs = [
          Tyxml_js.Html5.a_onclick(_ => {
            instance_click_fn(inst);
            true;
          }),
        ];

        let inst_cls = cls_of_inst(inst);
        term_with_attrs(
          prefix,
          err_status,
          rev_path,
          ["EmptyHole", "hole-instance", "selected-instance", inst_cls],
          attrs,
          r,
        );
      | NonEmptyHole(u, _, sigma, d1) =>
        let rev_path1 = [0, ...rev_path];
        let r1 =
          of_dhexp'(
            instance_click_fn,
            false,
            prefix,
            InHole(u),
            rev_path1,
            d1,
          );

        let r =
          dbg_SHOW_SIGMAS ?
            r1 ^^ of_sigma(instance_click_fn, prefix, rev_path, sigma) : r1;
        term(prefix, err_status, rev_path, "NonEmptyHole", r);
      | Cast(Cast(d1, ty1, ty2), ty3, ty4) when HTyp.eq(ty2, ty3) =>
        let rev_path1 = [0, ...rev_path];
        let inner_rev_path1 = [0, ...rev_path1];
        let inner_rev_path2 = [1, ...rev_path1];
        let inner_rev_path3 = [2, ...rev_path1];
        let rev_path3 = [2, ...rev_path];
        let paren1 = precedence_dhexp(d1) > precedence_const;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            inner_rev_path1,
            d1,
          );

        let r2 = of_htype(false, prefix, inner_rev_path2, ty1);
        let r3 = of_htype(false, prefix, inner_rev_path3, ty2);
        let r5 = of_htype(false, prefix, rev_path3, ty4);
        of_chained_Cast(prefix, err_status, rev_path, r1, r2, r3, r5);
      | Cast(d1, ty1, ty2) =>
        let rev_path1 = [0, ...rev_path];
        let rev_path2 = [1, ...rev_path];
        let rev_path3 = [2, ...rev_path];
        let paren1 = precedence_dhexp(d1) > precedence_const;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );

        let r2 = of_htype(false, prefix, rev_path2, ty1);
        let r3 = of_htype(false, prefix, rev_path3, ty2);
        of_Cast(prefix, err_status, rev_path, r1, r2, r3);
      | FailedCast(Cast(d1, ty1, ty2), ty3, ty4) when HTyp.eq(ty2, ty3) =>
        let rev_path1 = [0, ...rev_path];
        let inner_rev_path1 = [0, ...rev_path1];
        let inner_rev_path2 = [1, ...rev_path1];
        let inner_rev_path3 = [2, ...rev_path1];
        let rev_path3 = [2, ...rev_path];
        let paren1 = precedence_dhexp(d1) > precedence_const;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            inner_rev_path1,
            d1,
          );

        let r2 = of_htype(false, prefix, inner_rev_path2, ty1);
        let r3 = of_htype(false, prefix, inner_rev_path3, ty2);
        let r5 = of_htype(false, prefix, rev_path3, ty4);
        of_chained_FailedCast(prefix, err_status, rev_path, r1, r2, r3, r5);
      | FailedCast(d1, ty1, ty2) =>
        let rev_path1 = [0, ...rev_path];
        let rev_path2 = [1, ...rev_path];
        let rev_path3 = [2, ...rev_path];
        let paren1 = precedence_dhexp(d1) > precedence_const;
        let r1 =
          of_dhexp'(
            instance_click_fn,
            paren1,
            prefix,
            NotInHole,
            rev_path1,
            d1,
          );

        let r2 = of_htype(false, prefix, rev_path2, ty1);
        let r3 = of_htype(false, prefix, rev_path3, ty2);
        of_FailedCast(prefix, err_status, rev_path, r1, r2, r3);
      }
    );

  parenthesize ? lparen("(") ^^ doc ^^ rparen(")") : doc;
}
and of_sigma = (instance_click_fn, prefix, rev_path, sigma) => {
  let map_f = ((x, d)) =>
    of_dhexp'(instance_click_fn, false, prefix, NotInHole, rev_path, d)
    ^^ kw("/")
    ^^ PP.text("", x);

  let docs = List.map(map_f, sigma);
  let doc' =
    switch (docs) {
    | [] => PP.empty
    | [x, ...xs] =>
      let fold_f = (doc, doc') => doc ^^ kw(", ") ^^ doc';
      List.fold_left(fold_f, x, xs);
    };

  lparen("[") ^^ doc' ^^ rparen("]");
};

let of_dhexp = (instance_click_fn, prefix, d) =>
  of_dhexp'(instance_click_fn, false, prefix, NotInHole, [], d);
let html_of_ty = (width, prefix, ty) => {
  let ty_doc = of_htype(false, prefix, [], ty);
  let rev_paths = EditorBoxTypes.mk_rev_paths();
  let ty_sdoc = Pretty.PP.sdoc_of_doc(width, ty_doc, rev_paths);
  Pretty.HTML_Of_SDoc.html_of_sdoc(ty_sdoc, rev_paths);
};
let html_of_dhexp = (instance_click_fn, width, prefix, d) => {
  let dhexp_doc = of_dhexp(instance_click_fn, prefix, d);
  let rev_paths = EditorBoxTypes.mk_rev_paths();
  let dhexp_sdoc = Pretty.PP.sdoc_of_doc(width, dhexp_doc, rev_paths);
  Pretty.HTML_Of_SDoc.html_of_sdoc(dhexp_sdoc, rev_paths);
};
let html_of_var = (width, prefix, x) =>
  html_of_dhexp(_ => (), width, prefix, DHExp.BoundVar(x));

let html_of_hole_instance = (instance_click_fn, width, prefix, (u, i)) => {
  let d = Dynamics.DHExp.EmptyHole(u, i, []);
  html_of_dhexp(instance_click_fn, width, prefix, d);
};
let string_of_cursor_side = cursor_side =>
  switch (cursor_side) {
  | In(n) => "In(" ++ string_of_int(n) ++ ")"
  | Before => "Before"
  | After => "After"
  };
