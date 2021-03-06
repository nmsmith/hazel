open SemanticsCore;
type edit_state = ((ZExp.t, HTyp.t), MetaVar.gen);
let u_gen0: MetaVar.gen = (MetaVar.new_gen: MetaVar.gen);
let (u, u_gen1) = MetaVar.next(u_gen0);
let empty_ze =
  ZExp.CursorE(Before, UHExp.Tm(NotInHole, UHExp.EmptyHole(u)));
let empty: edit_state = (((empty_ze, HTyp.Hole), u_gen1): edit_state);
let empty_erasure = ZExp.erase(empty_ze);
type edit_state_rs = React.signal(edit_state);
type e_rs = React.signal(UHExp.t);
type cursor_info_rs = React.signal(ZExp.cursor_info);
open Dynamics;
type result_rs =
  React.signal((DHExp.t, DHExp.HoleInstanceInfo.t, Evaluator.result));
type hole_instance_info_rs = React.signal(DHExp.HoleInstanceInfo.t);
module UserSelectedInstances = {
  type t = MetaVarMap.t(DHExp.inst_num);
  type rs = React.signal(t);
  type rf = (~step: React.step=?, MetaVarMap.t(DHExp.inst_num)) => unit;
  let update = (usi, inst) => MetaVarMap.insert_or_update(usi, inst);
};
type instance_click_fn = DHExp.HoleInstance.t => unit;
type user_selected_instances_rs = UserSelectedInstances.rs;
type user_selected_instances_rf = UserSelectedInstances.rf;
type selected_instance_rs = React.signal(option(DHExp.HoleInstance.t));
type selected_instance_rf =
  (~step: React.step=?, option(DHExp.HoleInstance.t)) => unit;
type monitors = list(React.signal(unit));
type do_action_t = Action.t => unit;
exception InvalidAction;
exception MissingCursorInfo;
exception DoesNotExpand;
exception InvalidInput;
type t = {
  edit_state_rs,
  cursor_info_rs,
  e_rs,
  result_rs,
  user_selected_instances_rs,
  user_selected_instances_rf,
  selected_instance_rs,
  selected_instance_rf,
  monitors,
  do_action: do_action_t,
};
let new_model = (): t => {
  let (edit_state_rs, edit_state_rf) = React.S.create(empty);
  let (e_rs, e_rf) = React.S.create(empty_erasure);
  let cursor_info_rs =
    React.S.l1(
      ~eq=(_, _) => false, /* palette contexts have functions in them! */
      (((ze, _), _)) =>
        switch (
          ZExp.syn_cursor_info(
            (),
            (Ctx.empty, Palettes.initial_palette_ctx),
            ze,
          )
        ) {
        | Some(cursor_info) => cursor_info
        | None => raise(MissingCursorInfo)
        },
      edit_state_rs,
    );

  let result_rs =
    React.S.l1(
      e => {
        let expanded =
          DHExp.syn_expand((), (Ctx.empty, Palettes.initial_palette_ctx), e);
        switch (expanded) {
        | DHExp.DoesNotExpand => raise(DoesNotExpand)
        | DHExp.Expands(d, _, _) =>
          switch (Evaluator.evaluate((), d)) {
          | Evaluator.InvalidInput(n) =>
            JSUtil.log("InvalidInput " ++ string_of_int(n));
            raise(InvalidInput);
          | Evaluator.BoxedValue(d) =>
            let (d_renumbered, hii) =
              DHExp.renumber((), [], DHExp.HoleInstanceInfo.empty, d);
            (d_renumbered, hii, Evaluator.BoxedValue(d_renumbered));
          | Evaluator.Indet(d) =>
            let (d_renumbered, hii) =
              DHExp.renumber((), [], DHExp.HoleInstanceInfo.empty, d);
            (d_renumbered, hii, Evaluator.Indet(d_renumbered));
          }
        };
      },
      e_rs,
    );

  let (user_selected_instances_rs, user_selected_instances_rf) =
    React.S.create(MetaVarMap.empty);
  let usi_monitor =
    React.S.l1(_ => user_selected_instances_rf(MetaVarMap.empty), result_rs);

  let (selected_instance_rs, selected_instance_rf) = React.S.create(None);
  let instance_at_cursor_monitor =
    React.S.l2(
      ({ZExp.mode: _, ZExp.sort, ZExp.ctx: _, ZExp.side: _}, (_, hii, _)) => {
        let new_path =
          switch (sort) {
          | ZExp.IsExpr(UHExp.Tm(_, UHExp.EmptyHole(u))) =>
            let usi = React.S.value(user_selected_instances_rs);
            switch (MetaVarMap.lookup(usi, u)) {
            | Some(i) => Some((u, i))
            | None =>
              switch (DHExp.HoleInstanceInfo.default_instance(hii, u)) {
              | Some(_) as inst => inst
              | None => None
              }
            };
          | _ => None
          };
        selected_instance_rf(new_path);
      },
      cursor_info_rs,
      result_rs,
    );

  let monitors = [instance_at_cursor_monitor, usi_monitor];
  let do_action = action =>
    switch (
      Action.performSyn(
        (),
        (Ctx.empty, Palettes.initial_palette_ctx),
        action,
        React.S.value(edit_state_rs),
      )
    ) {
    | Some(((ze, ty), ugen)) =>
      edit_state_rf(((ze, ty), ugen));
      switch (action) {
      | Action.MoveTo(_)
      | Action.MoveToNextHole
      | Action.MoveToPrevHole => ()
      | _ => e_rf(ZExp.erase(ze))
      };
    | None => raise(InvalidAction)
    };
  {
    edit_state_rs,
    cursor_info_rs,
    e_rs,
    result_rs,
    user_selected_instances_rs,
    user_selected_instances_rf,
    selected_instance_rs,
    selected_instance_rf,
    monitors,
    do_action,
  };
};
